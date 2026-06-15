#include "ui/ParameterPage.h"

#include <QHeaderView>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QTableView>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <utility>

#include "ui/delegates/ActionButtonDelegate.h"

namespace
{
	void applyUniformColumnWidths(QTableView* tableView)
	{
		if (tableView == nullptr || tableView->model() == nullptr)
		{
			return;
		}

		const int columnCount = tableView->model()->columnCount();
		if (columnCount <= 0)
		{
			return;
		}

		const int availableWidth = qMax(1200, tableView->viewport()->width());
		const int uniformWidth   = qMax(110, availableWidth / columnCount);
		for (int column = 0; column < columnCount; ++column)
		{
			tableView->setColumnWidth(column, uniformWidth);
		}
	}
}

ParameterPage::ParameterPage(QWidget* parent)
	: QWidget(parent)
	  , m_categoryTree(new QTreeWidget(this))
	  , m_searchEdit(new QLineEdit(this))
	  , m_summaryLabel(new QLabel(this))
	  , m_tableView(new QTableView(this))
	  , m_tableModel(new ParameterTableModel(this))
	  , m_filterModel(new ParameterFilterModel(this))
	  , m_actionDelegate(new ActionButtonDelegate(this))
{
	auto* rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSpacing(12);

	auto* headerLayout = new QHBoxLayout();
	auto* titleLabel   = new QLabel(QStringLiteral("参数配置中心"), this);
	titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 700;"));
	m_searchEdit->setPlaceholderText(QStringLiteral("搜索参数名称、子命令、备注"));
	m_summaryLabel->setMinimumWidth(160);

	headerLayout->addWidget(titleLabel);
	headerLayout->addStretch(1);
	headerLayout->addWidget(m_searchEdit, 1);
	headerLayout->addWidget(m_summaryLabel);

	auto* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setChildrenCollapsible(false);

	m_categoryTree->setHeaderHidden(true);
	m_categoryTree->setMinimumWidth(220);
	m_categoryTree->setStyleSheet(QStringLiteral(
		"QTreeWidget { border: 1px solid #d9e2ec; border-radius: 10px; background: #f8fbfd; }"
		"QTreeWidget::item { padding: 8px 10px; }"
		"QTreeWidget::item:selected { background: #dbeafe; color: #123; border-radius: 6px; }"));

	m_filterModel->setSourceModel(m_tableModel);
	m_filterModel->setFilterKeyColumn(-1);

	m_tableView->setModel(m_filterModel);
	m_tableView->setAlternatingRowColors(true);
	m_tableView->setSortingEnabled(false);
	m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tableView->verticalHeader()->setVisible(false);
	m_tableView->horizontalHeader()->setStretchLastSection(false);
	m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_tableView->horizontalHeader()->setSectionsMovable(true);
	m_tableView->horizontalHeader()->setSectionsClickable(true);
	m_tableView->horizontalHeader()->setSortIndicatorShown(false);
	m_tableView->horizontalHeader()->setMinimumSectionSize(90);
	m_tableView->horizontalHeader()->setDefaultSectionSize(120);
	m_tableView->setStyleSheet(QStringLiteral(
		"QTableView { border: 1px solid #d9e2ec; border-radius: 10px; background: white; gridline-color: #edf2f7; }"
		"QHeaderView::section { background: #eff6ff; padding: 8px; border: none; border-right: 1px solid #dbeafe; }"));

	m_actionDelegate->setActionHandler([this](int proxyRow, ParameterAction action)
	{
		handleRowAction(proxyRow, action);
	});
	m_tableView->setItemDelegateForColumn(ParameterTableModel::ActionColumn, m_actionDelegate);

	splitter->addWidget(m_categoryTree);
	splitter->addWidget(m_tableView);
	splitter->setStretchFactor(1, 1);

	rootLayout->addLayout(headerLayout);
	rootLayout->addWidget(splitter, 1);

	QObject::connect(m_categoryTree, &QTreeWidget::currentItemChanged, this,
	                 [this](QTreeWidgetItem* current, QTreeWidgetItem* previous)
	                 {
		                 handleCategoryChanged(current, previous);
	                 });
	QObject::connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		handleSearchChanged(text);
	});

	setStyleSheet(QStringLiteral("background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f4f7fb, stop:1 #eef8f3);"));
}

void ParameterPage::setRecords(const QVector<ParameterRecord>& records)
{
	m_tableModel->setRecords(records);
	applyUniformColumnWidths(m_tableView);
	rebuildCategoryTree(records);
	m_summaryLabel->setText(
		QStringLiteral("当前 %1 / 总计 %2").arg(m_filterModel->rowCount()).arg(m_tableModel->rowCount()));
}

void ParameterPage::setActionHandler(std::function<void(const ParameterRecord&, ParameterAction)> handler)
{
	m_actionHandler = std::move(handler);
}

void ParameterPage::rebuildCategoryTree(const QVector<ParameterRecord>& records)
{
	m_categoryTree->clear();
	auto* allItem = new QTreeWidgetItem(QStringList(QStringLiteral("全部参数")));
	allItem->setData(0, Qt::UserRole, QStringLiteral("全部参数"));
	m_categoryTree->addTopLevelItem(allItem);

	QStringList categories;
	for (const ParameterRecord& record : records)
	{
		if (categories.contains(record.groupName))
		{
			continue;
		}
		categories.push_back(record.groupName);
		auto* item = new QTreeWidgetItem(QStringList(record.groupName));
		item->setData(0, Qt::UserRole, record.groupName);
		m_categoryTree->addTopLevelItem(item);
	}

	m_categoryTree->setCurrentItem(allItem);
}

void ParameterPage::handleCategoryChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	Q_UNUSED(previous);

	if (current == nullptr)
	{
		return;
	}

	m_filterModel->setCategoryFilter(current->data(0, Qt::UserRole).toString());
	m_summaryLabel->setText(
		QStringLiteral("当前 %1 / 总计 %2").arg(m_filterModel->rowCount()).arg(m_tableModel->rowCount()));
}

void ParameterPage::handleSearchChanged(const QString& text)
{
	m_filterModel->setFilterFixedString(text);
	m_summaryLabel->setText(
		QStringLiteral("当前 %1 / 总计 %2").arg(m_filterModel->rowCount()).arg(m_tableModel->rowCount()));
}

void ParameterPage::handleRowAction(int proxyRow, ParameterAction action)
{
	if (!m_actionHandler)
	{
		return;
	}

	const QModelIndex proxyIndex  = m_filterModel->index(proxyRow, 0);
	const QModelIndex sourceIndex = m_filterModel->mapToSource(proxyIndex);
	if (!sourceIndex.isValid())
	{
		return;
	}

	m_actionHandler(m_tableModel->recordAt(sourceIndex.row()), action);
}
