#include "protocolfieldtablemodel.h"

namespace {
// 类型显示名，与 stringToType 保持一一对应。
// 注意顺序：与 FieldType 枚举值顺序无关，但两个函数需一致。
struct TypeEntry {
    FieldType type;
    const char *name;
};
const TypeEntry kTypeEntries[] = {
    { FieldType::UInt8,     "UInt8"     },
    { FieldType::UInt16,    "UInt16"    },
    { FieldType::UInt32,    "UInt32"    },
    { FieldType::Int8,      "Int8"      },
    { FieldType::Int16,     "Int16"     },
    { FieldType::Int32,     "Int32"     },
    { FieldType::Float32,   "Float32"   },
    { FieldType::Float64,   "Float64"   },
    { FieldType::ByteArray, "ByteArray" },
    { FieldType::BitField,  "BitField"  },
    { FieldType::String,    "String"    },
    { FieldType::Struct,    "Struct"    },
};
constexpr int kTypeEntryCount = int(sizeof(kTypeEntries) / sizeof(kTypeEntries[0]));
} // namespace

QString ProtocolFieldTableModel::typeToString(FieldType t)
{
    for (int i = 0; i < kTypeEntryCount; ++i) {
        if (kTypeEntries[i].type == t) {
            return QString::fromLatin1(kTypeEntries[i].name);
        }
    }
    return QStringLiteral("UInt8");
}

FieldType ProtocolFieldTableModel::stringToType(const QString &s, bool *ok)
{
    for (int i = 0; i < kTypeEntryCount; ++i) {
        if (s.compare(QString::fromLatin1(kTypeEntries[i].name), Qt::CaseInsensitive) == 0) {
            if (ok) *ok = true;
            return kTypeEntries[i].type;
        }
    }
    if (ok) *ok = false;
    return FieldType::UInt8;
}

ProtocolFieldTableModel::ProtocolFieldTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ProtocolFieldTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_fields.size();
}

int ProtocolFieldTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColCount;
}

QVariant ProtocolFieldTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_fields.size()) {
        return {};
    }
    const FieldDef &f = m_fields.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case NameCol:      return f.name;
        case TypeCol:      return typeToString(f.type);
        case OffsetCol:    return quint32(f.byteOffset);
        case LengthCol:    return quint32(f.byteLength);
        case BitOffsetCol: return quint32(f.bitOffset);
        case BitLengthCol: return quint32(f.bitLength);
        case CommentCol:   return f.comment;
        default: break;
        }
    }
    return {};
}

QVariant ProtocolFieldTableModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case NameCol:      return QStringLiteral("名称");
    case TypeCol:      return QStringLiteral("类型");
    case OffsetCol:    return QStringLiteral("字节偏移");
    case LengthCol:    return QStringLiteral("字节长度");
    case BitOffsetCol: return QStringLiteral("位偏移");
    case BitLengthCol: return QStringLiteral("位长");
    case CommentCol:   return QStringLiteral("备注");
    default: return {};
    }
}

Qt::ItemFlags ProtocolFieldTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    // 偏移列只读（由布局自动计算或显式指定），其余可编辑。
    switch (index.column()) {
    case OffsetCol:
        break;
    default:
        f |= Qt::ItemIsEditable;
        break;
    }
    return f;
}

bool ProtocolFieldTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid()
        || index.row() < 0 || index.row() >= m_fields.size()) {
        return false;
    }
    FieldDef &f = m_fields[index.row()];
    switch (index.column()) {
    case NameCol:      f.name = value.toString().trimmed(); break;
    case TypeCol: {
        bool ok = false;
        const FieldType t = stringToType(value.toString(), &ok);
        if (!ok) return false;
        f.type = t;
        break;
    }
    case OffsetCol:    return false; // 只读
    case LengthCol:    f.byteLength = quint16(value.toUInt()); break;
    case BitOffsetCol: f.bitOffset = quint8(value.toUInt()); break;
    case BitLengthCol: f.bitLength = quint8(value.toUInt()); break;
    case CommentCol:   f.comment = value.toString(); break;
    default: return false;
    }
    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

void ProtocolFieldTableModel::setFields(const QVector<FieldDef> &fields)
{
    beginResetModel();
    m_fields = fields;
    endResetModel();
}

FieldDef ProtocolFieldTableModel::fieldAt(int row) const
{
    if (row < 0 || row >= m_fields.size()) {
        return {};
    }
    return m_fields.at(row);
}

void ProtocolFieldTableModel::addField(const FieldDef &field)
{
    const int row = m_fields.size();
    beginInsertRows(QModelIndex(), row, row);
    m_fields.append(field);
    endInsertRows();
}

void ProtocolFieldTableModel::removeRows(int row, int count)
{
    if (row < 0 || row >= m_fields.size() || count <= 0) {
        return;
    }
    const int end = qMin(row + count - 1, m_fields.size() - 1);
    beginRemoveRows(QModelIndex(), row, end);
    for (int i = end; i >= row; --i) {
        m_fields.removeAt(i);
    }
    endRemoveRows();
}

void ProtocolFieldTableModel::moveRow(int row, bool up)
{
    if (row < 0 || row >= m_fields.size()) {
        return;
    }
    const int target = up ? row - 1 : row + 1;
    if (target < 0 || target >= m_fields.size()) {
        return;
    }
    m_fields.swapItemsAt(row, target);
    // 简化刷新：通知两行都变化。
    const QModelIndex a = this->index(row, 0);
    const QModelIndex b = this->index(target, ColCount - 1);
    emit dataChanged(a, b, { Qt::DisplayRole, Qt::EditRole });
}
