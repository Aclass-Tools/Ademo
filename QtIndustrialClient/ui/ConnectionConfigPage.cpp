#include "ui/ConnectionConfigPage.h"

#include <utility>

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHostAddress>
#include "ui_ConnectionConfigPage.h"

ConnectionConfigPage::ConnectionConfigPage(QWidget* parent)
	: QWidget(parent)
	  , m_ui(std::make_unique<Ui::ConnectionConfigPage>())
{
	m_ui->setupUi(this);
	m_serialTable = m_ui->serialTable;
	m_ipEdit      = m_ui->lineEditIp;
	m_portEdit    = m_ui->lineEditPort;

	if (m_serialTable != nullptr)
	{
		m_serialTable->verticalHeader()->setVisible(false);
		m_serialTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		m_serialTable->setStyleSheet(QStringLiteral(
			"QTableWidget { border: none; background: white; gridline-color: #d9e2ec; }"
			"QHeaderView::section { background: #a8d3e3; color: #0f172a; padding: 6px; border: 1px solid #7faec0; }"));

		auto* portCellWidget = new QWidget(m_serialTable);
		auto* portLayout     = new QHBoxLayout(portCellWidget);
		auto* refreshButton  = new QPushButton(QStringLiteral("刷新"), portCellWidget);
		auto* portCombo      = new QComboBox(portCellWidget);
		auto* baudCombo      = new QComboBox(m_serialTable);
		auto* parityCombo    = new QComboBox(m_serialTable);
		auto* dataBitsCombo  = new QComboBox(m_serialTable);
		auto* stopBitsCombo  = new QComboBox(m_serialTable);
		m_boardIdEdit        = new QLineEdit(QStringLiteral("1"), m_serialTable);
		m_parentIdEdit       = new QLineEdit(QStringLiteral("0"), m_serialTable);
		m_portCombo          = portCombo;

		portLayout->setContentsMargins(2, 0, 2, 0);
		portLayout->setSpacing(4);
		refreshButton->setMinimumWidth(54);
		refreshButton->setMinimumHeight(30);
		portCombo->setMinimumHeight(30);
		portLayout->addWidget(refreshButton);
		portLayout->addWidget(portCombo, 1);

		baudCombo->addItems({QStringLiteral("9600"), QStringLiteral("115200"), QStringLiteral("460800")});
		baudCombo->setCurrentText(QStringLiteral("460800"));
		parityCombo->addItems({QStringLiteral("NoParity"), QStringLiteral("EvenParity"), QStringLiteral("OddParity")});
		dataBitsCombo->addItems({QStringLiteral("8"), QStringLiteral("7")});
		stopBitsCombo->addItems({QStringLiteral("OneStop"), QStringLiteral("TwoStop")});
		m_boardIdEdit->setReadOnly(true);
		m_parentIdEdit->setReadOnly(true);

		m_baudRateCombo = baudCombo;
		m_parityCombo   = parityCombo;
		m_dataBitsCombo = dataBitsCombo;
		m_stopBitsCombo = stopBitsCombo;

		connect(refreshButton, &QPushButton::clicked, this, [this]()
		{
			if (m_refreshPortsCallback)
			{
				m_refreshPortsCallback();
			}
		});

		m_serialTable->setCellWidget(0, 0, portCellWidget);
		m_serialTable->setCellWidget(0, 1, baudCombo);
		m_serialTable->setCellWidget(0, 2, parityCombo);
		m_serialTable->setCellWidget(0, 3, dataBitsCombo);
		m_serialTable->setCellWidget(0, 4, stopBitsCombo);
		m_serialTable->setCellWidget(0, 5, m_boardIdEdit);
		m_serialTable->setCellWidget(0, 6, m_parentIdEdit);
	}

	if (m_portEdit != nullptr)
	{
		m_portEdit->setReadOnly(true);
	}
	if (m_ipEdit != nullptr)
	{
		auto* hostValidator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[A-Za-z0-9.-]*$")),
		                                                      m_ipEdit);
		m_ipEdit->setValidator(hostValidator);
	}
	if (m_ui->labelTip != nullptr)
	{
		m_ui->labelTip->setStyleSheet(QStringLiteral("color: #64748b;"));
	}

	connect(m_ui->comboBoxcommiute, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &ConnectionConfigPage::onCommuteModeChanged);
	onCommuteModeChanged(m_ui->comboBoxcommiute->currentIndex());
}

ConnectionConfigPage::~ConnectionConfigPage() = default;

namespace
{
	bool isValidHostName(const QString& host)
	{
		if (host.isEmpty() || host.size() > 253)
		{
			return false;
		}

		const QStringList labels = host.split('.');
		for (const QString& label : labels)
		{
			if (label.isEmpty() || label.size() > 63)
			{
				return false;
			}
			if (!label.front().isLetterOrNumber() || !label.back().isLetterOrNumber())
			{
				return false;
			}
			for (const QChar ch : label)
			{
				if (!ch.isLetterOrNumber() && ch != QLatin1Char('-'))
				{
					return false;
				}
			}
		}
		return true;
	}
}

QString ConnectionConfigPage::selectedPortName() const
{
	if (m_portCombo == nullptr)
	{
		return {};
	}
	return m_portCombo->currentText();
}

int ConnectionConfigPage::selectedBaudRate() const
{
	return m_baudRateCombo == nullptr ? 115200 : m_baudRateCombo->currentText().toInt();
}

QString ConnectionConfigPage::selectedParity() const
{
	return m_parityCombo == nullptr ? QStringLiteral("NoParity") : m_parityCombo->currentText();
}

int ConnectionConfigPage::selectedDataBits() const
{
	return m_dataBitsCombo == nullptr ? 8 : m_dataBitsCombo->currentText().toInt();
}

QString ConnectionConfigPage::selectedStopBits() const
{
	return m_stopBitsCombo == nullptr ? QStringLiteral("OneStop") : m_stopBitsCombo->currentText();
}

int ConnectionConfigPage::boardId() const
{
	return m_boardIdEdit == nullptr ? 1 : m_boardIdEdit->text().toInt();
}

int ConnectionConfigPage::parentId() const
{
	return m_parentIdEdit == nullptr ? 0 : m_parentIdEdit->text().toInt();
}

QString ConnectionConfigPage::selectedIpAddress() const
{
	return m_ipEdit == nullptr ? QString() : m_ipEdit->text().trimmed();
}

int ConnectionConfigPage::selectedNetworkPort() const
{
	return m_portEdit == nullptr ? 0 : m_portEdit->text().toInt();
}

QString ConnectionConfigPage::currentTransportMode() const
{
	return m_ui->comboBoxcommiute->currentIndex() == 0 ? QStringLiteral("串口") : QStringLiteral("网口");
}

bool ConnectionConfigPage::validateNetworkAddress(QString* errorMessage) const
{
	const QString host = selectedIpAddress().trimmed();
	if (host.isEmpty())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("网址不能为空。");
		}
		return false;
	}

	QHostAddress parsedAddress;
	if (parsedAddress.setAddress(host))
	{
		return true;
	}

	if (!isValidHostName(host))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("网址编写错误，请输入有效的 IPv4 地址或域名。");
		}
		return false;
	}
	return true;
}

void ConnectionConfigPage::setSerialConfig(const QString& portName, int baudRate, const QString& parity, int dataBits,
                                           const QString& stopBits)
{
	if (m_portCombo != nullptr && !portName.isEmpty())
	{
		const int found = m_portCombo->findText(portName);
		if (found >= 0)
		{
			m_portCombo->setCurrentIndex(found);
		}
	}

	if (m_baudRateCombo != nullptr)
	{
		const int found = m_baudRateCombo->findText(QString::number(baudRate));
		if (found >= 0)
		{
			m_baudRateCombo->setCurrentIndex(found);
		}
	}

	if (m_parityCombo != nullptr)
	{
		const int found = m_parityCombo->findText(parity);
		if (found >= 0)
		{
			m_parityCombo->setCurrentIndex(found);
		}
	}

	if (m_dataBitsCombo != nullptr)
	{
		const int found = m_dataBitsCombo->findText(QString::number(dataBits));
		if (found >= 0)
		{
			m_dataBitsCombo->setCurrentIndex(found);
		}
	}

	if (m_stopBitsCombo != nullptr)
	{
		const int found = m_stopBitsCombo->findText(stopBits);
		if (found >= 0)
		{
			m_stopBitsCombo->setCurrentIndex(found);
		}
	}
}

void ConnectionConfigPage::setNetworkConfig(const QString& ipAddress, int port)
{
	if (m_ipEdit != nullptr && !ipAddress.isEmpty())
	{
		m_ipEdit->setText(ipAddress);
	}
	if (m_portEdit != nullptr && port > 0)
	{
		m_portEdit->setText(QString::number(port));
	}
}

void ConnectionConfigPage::setTransportMode(const QString& mode)
{
	const bool serialMode = (mode == QStringLiteral("串口")) || (mode.compare(
		QStringLiteral("serial"), Qt::CaseInsensitive) == 0);
	m_ui->comboBoxcommiute->setCurrentIndex(serialMode ? 0 : 1);
}

void ConnectionConfigPage::onCommuteModeChanged(int index)
{
	m_ui->stackedWidget->setCurrentIndex(index);

	// 触发配置更改信号，通知MainWindow通信模式已改变
	emit transportModeChanged(index == 0 ? QStringLiteral("串口") : QStringLiteral("网口"));
}

void ConnectionConfigPage::refreshPorts(const QStringList& ports)
{
	if (m_portCombo == nullptr)
	{
		return;
	}
	const QString current = m_portCombo->currentText();
	m_portCombo->clear();
	m_portCombo->addItems(ports);
	const int index = m_portCombo->findText(current);
	if (index >= 0)
	{
		m_portCombo->setCurrentIndex(index);
		return;
	}
	if (m_portCombo->count() > 0)
	{
		m_portCombo->setCurrentIndex(0);
	}
}

void ConnectionConfigPage::setRefreshPortsCallback(std::function<void()> callback)
{
	m_refreshPortsCallback = std::move(callback);
}

QComboBox* ConnectionConfigPage::commuteComboBox() const
{
	return m_ui->comboBoxcommiute;
}
