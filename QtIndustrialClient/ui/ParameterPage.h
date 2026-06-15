#pragma once

#include <functional>

#include <QWidget>

#include "models/ParameterFilterModel.h"
#include "models/ParameterTableModel.h"

class ActionButtonDelegate;
class QLabel;
class QLineEdit;
class QTableView;
class QTreeWidget;
class QTreeWidgetItem;

class ParameterPage : public QWidget {
public:
    explicit ParameterPage(QWidget* parent = nullptr);

    void setRecords(const QVector<ParameterRecord>& records);
    void setActionHandler(std::function<void(const ParameterRecord&, ParameterAction)> handler);

private:
    void rebuildCategoryTree(const QVector<ParameterRecord>& records);
    void handleCategoryChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void handleSearchChanged(const QString& text);
    void handleRowAction(int proxyRow, ParameterAction action);

    QTreeWidget* m_categoryTree = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QTableView* m_tableView = nullptr;
    ParameterTableModel* m_tableModel = nullptr;
    ParameterFilterModel* m_filterModel = nullptr;
    ActionButtonDelegate* m_actionDelegate = nullptr;
    std::function<void(const ParameterRecord&, ParameterAction)> m_actionHandler;
};
