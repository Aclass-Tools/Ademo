#pragma once

#include <functional>
#include <memory>

#include <QWidget>

class QComboBox;
class QLineEdit;
class QStackedWidget;
class QTableWidget;

namespace Ui
{
	class ConnectionPage;
}

class ConnectionPage : public QWidget
{
	Q_OBJECT

public:
	explicit ConnectionPage(QWidget* parent = nullptr);
	~ConnectionPage() override;

	QString selectedPortName() const;
	int     selectedBaudRate() const;
	QString selectedParity() const;
	int     selectedDataBits() const;
	QString selectedStopBits() const;
	QString selectedIpAddress() const;
	int     selectedNetworkPort() const;
	QString currentTransportMode() const;

	void setSerialConfig(const QString& portName, int baudRate, const QString& parity, int dataBits,
	                     const QString& stopBits);
	void setNetworkConfig(const QString& ipAddress, int port);
	void setTransportMode(const QString& mode);


signals:
	void transportModeChanged(const QString& mode);

private slots:
	void onCommuteModeChanged(int index);
	void refreshPorts();

private:
	QTableWidget* m_serialTable = nullptr;
	QComboBox*    m_portCombo   = nullptr;
	// QComboBox* m_baudRateCombo = nullptr;
	// QComboBox*                          m_parityCombo     = nullptr;
	// QComboBox*                          m_dataBitsCombo   = nullptr;
	// QComboBox*                          m_stopBitsCombo   = nullptr;
	QLineEdit*                          m_ipEdit          = nullptr;
	QLineEdit*                          m_portEdit        = nullptr;
	QStackedWidget*                     m_connectionStack = nullptr;
	std::unique_ptr<Ui::ConnectionPage> m_ui;
};
