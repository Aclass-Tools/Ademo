#include "models/ParameterTableModel.h"

#include <utility>

namespace {
QString joinEnumValues(const QStringList& values)
{
    return values.join(" | ");
}

QString formatSubCommand(int commandId)
{
    return QStringLiteral("0x%1").arg(commandId, 2, 16, QLatin1Char('0')).toUpper();
}
}

ParameterTableModel::ParameterTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ParameterTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_records.size();
}

int ParameterTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant ParameterTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size()) {
        return {};
    }

    const ParameterRecord& record = m_records.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ParameterNameColumn:
            return record.parameterName;
        case SubCommandColumn:
            return formatSubCommand(record.commandId);
        case PermissionColumn:
            return record.permission;
        case ReadIndexColumn:
            return record.readIndex;
        case DataTypeColumn:
            return record.dataType;
        case DataLengthColumn:
            return record.dataLength;
        case DescriptionColumn:
            return record.description;
        case DefaultValueColumn:
            return record.defaultValue;
        case UnitColumn:
            return record.unit;
        case StorageColumn:
            return record.storageLocation;
        case MaxValueColumn:
            return record.maxValue;
        case MinValueColumn:
            return record.minValue;
        case StepColumn:
            return record.step;
        case EnumColumn:
            return joinEnumValues(record.enumValues);
        case RemarkColumn:
            return record.remark;
        case ActionColumn:
            return record.permission;
        default:
            return {};
        }
    }

    if (role == Qt::ToolTipRole) {
        return QString("%1 / %2\n%3").arg(record.groupName, record.commandName, record.remark);
    }

    if (role == Qt::UserRole) {
        return record.groupName;
    }

    if (role == Qt::UserRole + 1) {
        return record.permission;
    }

    if (role == Qt::TextAlignmentRole && (index.column() == ReadIndexColumn || index.column() == DataLengthColumn)) {
        return static_cast<int>(Qt::AlignCenter);
    }

    return {};
}

QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
    case ParameterNameColumn:
        return QStringLiteral("参数名称");
    case SubCommandColumn:
        return QStringLiteral("子命令");
    case PermissionColumn:
        return QStringLiteral("权限");
    case ReadIndexColumn:
        return QStringLiteral("读索引");
    case DataTypeColumn:
        return QStringLiteral("数据类型");
    case DataLengthColumn:
        return QStringLiteral("数据长度");
    case DescriptionColumn:
        return QStringLiteral("说明");
    case DefaultValueColumn:
        return QStringLiteral("默认值");
    case UnitColumn:
        return QStringLiteral("单位");
    case StorageColumn:
        return QStringLiteral("存储位置");
    case MaxValueColumn:
        return QStringLiteral("上限值");
    case MinValueColumn:
        return QStringLiteral("下限值");
    case StepColumn:
        return QStringLiteral("步进");
    case EnumColumn:
        return QStringLiteral("枚举");
    case RemarkColumn:
        return QStringLiteral("备注");
    case ActionColumn:
        return QStringLiteral("操作");
    default:
        return {};
    }
}

void ParameterTableModel::setRecords(QVector<ParameterRecord> records)
{
    beginResetModel();
    m_records = std::move(records);
    endResetModel();
}

const ParameterRecord& ParameterTableModel::recordAt(int row) const
{
    return m_records.at(row);
}
