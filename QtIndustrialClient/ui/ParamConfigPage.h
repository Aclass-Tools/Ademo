#pragma once

#include <functional>
#include <QWidget>
#include <memory>
#include <QJsonObject>

#include "models/ParamConfigure.h"
#include "models/ParameterRecord.h"

class ProtocolConfigPage;
class ConnectionConfigPage;

namespace Ui
{
	class ParamConfigPageClass;
}

/**
 * @brief 参数配置页面 - 整合协议配置和连接配置
 *
 * 包含协议配置（ProtocolConfigPage）和连接配置（ConnectionConfigPage）
 * 保存/恢复功能与连接信息序列化
 */
class ParamConfigPage : public QWidget
{
	Q_OBJECT

public:
	explicit ParamConfigPage(QWidget* parent = nullptr);
	~ParamConfigPage() override;

	// Protocol configuration methods
	void setRecords(const QVector<ParameterRecord>& records);
	QVector<ParameterRecord> records() const;
	bool checkForDuplicateSubCommands();

	// Connection configuration methods
	QString selectedPortName() const;
	int selectedBaudRate() const;
	QString selectedIpAddress() const;
	int selectedNetworkPort() const;
	QString currentTransportMode() const;
	int boardId() const;
	int parentId() const;
	void refreshPorts(const QStringList& ports);
	void setRefreshPortsCallback(std::function<void()> callback);
	void showProtocolPage();
	void showConnectionPage();
	bool validateBeforeLeave(bool showMessage) const;

	// Save/Load configuration with connection info
	bool saveConfig(const QString& filePath);
	bool loadConfig(const QString& filePath);

signals:
	void saveSuccess();
	void saveFailed(const QString& error);
	void transportModeChanged(const QString& mode);

private slots:
	void on_saveButton_clicked();
	void on_restoreButton_clicked();
	void onContentTabChanged(int index);

private:
	ParamComminicate serializeConnectionConfig() const;
	void             deserializeConnectionConfig(const ParamComminicate& comminicate);

	ProtocolConfigPage* m_protocolPage;
	ConnectionConfigPage* m_connectionPage;
	std::unique_ptr<Ui::ParamConfigPageClass> m_ui;
	int m_lastTabIndex = 0;
	bool m_switchingTab = false;
};
