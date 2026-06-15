#pragma once

#include <memory>

#include <QMap>
#include <QVector>
#include <QWidget>

#include "models/ParameterRecord.h"

class QTabWidget;
class QTableWidget;
class ComboBoxDelegate;
class RemarkComboDelegate;

namespace Ui
{
	class ProtocolConfigPage;
}

class ProtocolConfigPage : public QWidget
{
	Q_OBJECT

public:
	explicit ProtocolConfigPage(QWidget* parent = nullptr);
	~ProtocolConfigPage() override;

	void                     setRecords(const QVector<ParameterRecord>& records);
	QVector<ParameterRecord> records() const;

	bool saveConfig(const QString& filePath);
	bool loadConfig(const QString& filePath);
	bool checkForDuplicateSubCommands();
	void saveProtocolConfigure();
	void restoreProtocolConfigure();

signals:
	void saveSuccess();
	void saveFailed(const QString& error);

private slots:
	void on_btnAddMainOrder_clicked();
	void on_btnDeleteMainOrder_clicked();
	void on_btnAddSubOrder_clicked();
	void on_btnClearAllOrder_clicked();
	void on_btnDeleteSubOrder_clicked();

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QTabWidget*   createProtocolTabs(QWidget* parent);
	QTableWidget* createTable(QWidget* parent);
	void          populateRow(QTableWidget* table, int row, const ParameterRecord& record);
	QString       groupTabTitle(const QString& groupName, int groupId);

	QVector<ParameterRecord> collectRecordsFromTables() const;
	QTableWidget*            findTableAt(int protocolIndex, int groupIndex) const;
	QStringList              getPermissionOptions();
	QStringList              getDataTypeOptions();
	void                     renameMainOrderTab(QTabWidget* protocolTabs, int tabIndex);

	QTabWidget*                             m_groupTabs = nullptr;
	QVector<ParameterRecord>                m_records;
	std::unique_ptr<Ui::ProtocolConfigPage> m_ui;
	ComboBoxDelegate*                       m_permissionDelegate = nullptr;
	ComboBoxDelegate*                       m_dataTypeDelegate   = nullptr;
	RemarkComboDelegate*                    m_remarkDelegate     = nullptr;
};
