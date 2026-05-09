#include "pointtablemodel.h"

PointTableModel::PointTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PointTableModel::rowCount(const QModelIndex &parent) const
{
    // 扁平表模型，不支持子节点行。
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

int PointTableModel::columnCount(const QModelIndex &parent) const
{
    // 当前占位实现为固定 5 列。
    if (parent.isValid()) {
        return 0;
    }
    return 5;
}

QVariant PointTableModel::data(const QModelIndex &index, int role) const
{
    // 防御性边界校验。
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const PointItem &item = m_items.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        // 显示态与编辑态使用同一份源值。
        switch (index.column()) {
        case 0: return item.id;
        case 1: return item.version;
        case 2: return item.name;
        case 3: return item.protocol;
        case 4: return item.registerAddress;
        default: break;
        }
    }
    return {};
}

QVariant PointTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case 0: return QStringLiteral("ID");
    case 1: return QStringLiteral("Version");
    case 2: return QStringLiteral("Name");
    case 3: return QStringLiteral("Protocol");
    case 4: return QStringLiteral("RegisterAddr");
    default: return {};
    }
}

Qt::ItemFlags PointTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    // id/version 只读，可编辑列从第 2 列开始。
    if (index.column() >= 2) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}

bool PointTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // 仅允许 EditRole 写入。
    if (role != Qt::EditRole || !index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }

    PointItem &item = m_items[index.row()];
    switch (index.column()) {
    case 2:
        item.name = value.toString();
        break;
    case 3:
        item.protocol = value.toString();
        break;
    case 4:
        item.registerAddress = value.toInt();
        break;
    default:
        // 非可编辑列的写入请求直接拒绝。
        return false;
    }
    // 通知视图与代理模型刷新。
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

void PointTableModel::setItems(const QVector<PointItem> &items)
{
    // 全量 reset：实现简单，状态一致性高。
    beginResetModel();
    m_items = items;
    endResetModel();
}

const QVector<PointItem> &PointTableModel::items() const
{
    return m_items;
}

PointItem PointTableModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return {};
    }
    return m_items.at(row);
}

void PointTableModel::updateVersionForRow(int row, int newVersion)
{
    if (row < 0 || row >= m_items.size()) {
        return;
    }
    m_items[row].version = newVersion;
    // version 固定在第 1 列。
    const QModelIndex index = this->index(row, 1);
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}
