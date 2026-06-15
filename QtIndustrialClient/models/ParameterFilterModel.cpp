#include "models/ParameterFilterModel.h"

ParameterFilterModel::ParameterFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void ParameterFilterModel::setCategoryFilter(const QString& category)
{
    m_category = category;
    invalidateFilter();
}

bool ParameterFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const QModelIndex groupIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString groupName = sourceModel()->data(groupIndex, Qt::UserRole).toString();

    if (!m_category.isEmpty() && m_category != QStringLiteral("全部参数") && groupName != m_category) {
        return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
