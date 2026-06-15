#pragma once

#include <QSortFilterProxyModel>

class ParameterFilterModel : public QSortFilterProxyModel {
public:
    explicit ParameterFilterModel(QObject* parent = nullptr);

    void setCategoryFilter(const QString& category);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_category;
};
