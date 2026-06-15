#pragma once

#include <functional>
#include <memory>
#include <QStringList>
#include <QWidget>

class QComboBox;
class QLineEdit;
class QStackedWidget;
class QTableWidget;

namespace Ui
{
	class ConnectionConfigPage;
}

class ConnectionConfigPage : public QWidget
{
	Q_OBJECT

public:
	explicit ConnectionConfigPage(QWidget* parent = nullptr);
	~ConnectionConfigPage() override;

	QString selectedPortName() const;
	int selectedBaudRate() const;
	QString selectedParity() const;
	int selectedDataBits() const;
	QString selectedStopBits() const;
	int boardId() const;
	int parentId() const;
	QString selectedIpAddress() const;
	int selectedNetworkPort() const;
	QString currentTransportMode() const;
	bool validateNetworkAddress(QString* errorMessage = nullptr) const;
	void setSerialConfig(const QString& portName, int baudRate, const QString& parity, int dataBits, const QString& stopBits);
	void setNetworkConfig(const QString& ipAddress, int port);
	void setTransportMode(const QString& mode);
	void refreshPorts(const QStringList& ports);
	void setRefreshPortsCallback(std::function<void()> callback);
	QComboBox* commuteComboBox() const;

signals:
	void transportModeChanged(const QString& mode);

private slots:
	void onCommuteModeChanged(int index);

private:
	QTableWidget* m_serialTable = nullptr;
	QComboBox* m_portCombo = nullptr;
	QComboBox* m_baudRateCombo = nullptr;
	QComboBox* m_parityCombo = nullptr;
	QComboBox* m_dataBitsCombo = nullptr;
	QComboBox* m_stopBitsCombo = nullptr;
	QLineEdit* m_ipEdit = nullptr;
	QLineEdit* m_portEdit = nullptr;
	QLineEdit* m_boardIdEdit = nullptr;
	QLineEdit* m_parentIdEdit = nullptr;
	std::function<void()> m_refreshPortsCallback;
	std::unique_ptr<Ui::ConnectionConfigPage> m_ui;
};
