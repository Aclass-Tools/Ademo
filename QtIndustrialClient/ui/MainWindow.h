#pragma once

#include <memory>

#include <QMainWindow>

#include "models/ParameterRecord.h"
#include "services/NetworkService.h"
#include "services/SerialService.h"

class CommandOperationPage;
class ParamConfigPage;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTextEdit;
class ConnectionPage;

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
public:
	MainWindow();
	~MainWindow() override;
	void appendLog(const QString& message);

private:
	enum class ActiveConnectionMode
	{
		None,
		Serial,
		Network
	};

	void buildUi();
	void refreshPorts();
	void loadDefaultConfig();
	void loadConfigFromFile(const QString& filePath);
	void switchPage(int pageIndex);
	void toggleConnection();
	void handleParameterAction(const ParameterRecord& record, ParameterAction action);
	void updateConnectionBadge();

	/**
	 * @brief 更新绘图数据
	 * @param protocolType 协议类型
	 * @param groupId 分组ID
	 * @param commandId 命令ID
	 * @param payload 数据负载
	 */
	void updatePlotData(quint8 protocolType, int groupId, int commandId, const QByteArray& payload);

	/**
	 * @brief 解码数据值为double
	 * @param record 参数记录
	 * @param payload 负载数据
	 * @param offset 偏移量
	 * @return 解码后的double值
	 */
	double decodeValueToDouble(const ParameterRecord& record, const QByteArray& payload, int offset);

	ParamConfigPage*                m_paramConfigPage      = nullptr;
	CommandOperationPage*           m_commandOperationPage = nullptr;
	ConnectionPage*                 m_connectionPage       = nullptr;
	QStackedWidget*                 m_centerStack          = nullptr;
	QTableWidget*                   m_boardTable           = nullptr;
	QPushButton*                    m_connectButton        = nullptr;
	QTextEdit*                      m_logView              = nullptr;
	QString                         m_configPath;
	ActiveConnectionMode            m_activeConnectionMode = ActiveConnectionMode::None;
	SerialService                   m_serialService;
	NetworkService                  m_networkService;
	std::unique_ptr<Ui::MainWindow> m_ui;
};
