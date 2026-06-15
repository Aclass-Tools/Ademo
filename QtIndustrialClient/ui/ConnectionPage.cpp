#include "ui/ConnectionPage.h"
#include "ui_ConnectionPage.h"

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStackedWidget>
#include <QTableWidget>

ConnectionPage::ConnectionPage(QWidget* parent)
	: QWidget(parent)
	  , m_ui(std::make_unique<Ui::ConnectionPage>())
{
	m_ui->setupUi(this);
	m_connectionStack = m_ui->stackedWidget;
	m_serialTable     = m_ui->serialTable;
	m_ipEdit          = m_ui->lineEditIp;
	m_portEdit        = m_ui->lineEditPort;

	if (m_serialTable != nullptr)
	{
		m_serialTable->verticalHeader()->setVisible(false);
		m_serialTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		m_serialTable->setStyleSheet(QStringLiteral(
			"QTableWidget { border: none; background: white; gridline-color: #d9e2ec; }"
			"QHeaderView::section { background: #a8d3e3; color: #0f172a; padding: 6px; border: 1px solid #7faec0; }"));

		int rows = m_serialTable->rowCount();
		int cols = m_serialTable->columnCount();
		for (int row = 0; row < rows; row++)
		{
			for (int col = 0; col < cols; col++)
			{
				auto item = new QTableWidgetItem();
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
				m_serialTable->setItem(row, col, item);
			}
		}


		auto* portCellWidget = new QWidget(m_serialTable);
		auto* portLayout     = new QHBoxLayout(portCellWidget);
		auto* refreshButton  = new QPushButton(QStringLiteral("刷新"), portCellWidget);
		m_portCombo          = new QComboBox(portCellWidget);
		portLayout->setContentsMargins(2, 0, 2, 0);
		portLayout->setSpacing(4);
		refreshButton->setMinimumHeight(30);
		m_portCombo->setMinimumHeight(30);
		refreshButton->setMinimumWidth(54);
		portLayout->addWidget(refreshButton);
		portLayout->addWidget(m_portCombo, 1);
		m_serialTable->setCellWidget(0, 0, portCellWidget);
		m_serialTable->item(0, 1)->setText("115200");
		m_serialTable->item(0, 2)->setText("NoParity");
		m_serialTable->item(0, 3)->setText("8");
		m_serialTable->item(0, 4)->setText("OneStop");

		connect(refreshButton, &QPushButton::clicked, this, &ConnectionPage::refreshPorts);
	}

	if (m_portEdit != nullptr)
	{
		m_portEdit->setEnabled(false);
	}
	if (m_ui->labelTip != nullptr)
	{
		m_ui->labelTip->setStyleSheet(QStringLiteral("color: #64748b;"));
	}

	// connect(m_ui->comboBoxcommiute, QOverload<int>::of(&QComboBox::currentIndexChanged),
	//         this, &ConnectionPage::onCommuteModeChanged);
	// if (m_connectionStack != nullptr)
	// {
	// 	m_connectionStack->setCurrentIndex(m_ui->comboBoxcommiute->currentIndex());
	// }
}

ConnectionPage::~ConnectionPage() = default;

QString ConnectionPage::selectedPortName() const
{
	return m_portCombo == nullptr ? QString() : m_portCombo->currentText();
}

int ConnectionPage::selectedBaudRate() const
{
	return m_ui->serialTable->item(0, 1)->text().toInt();
}

QString ConnectionPage::selectedParity() const
{
	return m_ui->serialTable->item(0, 2)->text();
}

int ConnectionPage::selectedDataBits() const
{
	return m_ui->serialTable->item(0, 3)->text().toInt();
}

QString ConnectionPage::selectedStopBits() const
{
	return m_ui->serialTable->item(0, 4)->text();
}

QString ConnectionPage::selectedIpAddress() const
{
	return m_ipEdit == nullptr ? QString() : m_ipEdit->text().trimmed();
}

int ConnectionPage::selectedNetworkPort() const
{
	return m_portEdit == nullptr ? 0 : m_portEdit->text().toInt();
}

QString ConnectionPage::currentTransportMode() const
{
	return (m_connectionStack != nullptr && m_connectionStack->currentIndex() == 1)
	       ? QStringLiteral("网口")
	       : QStringLiteral("串口");
}

void ConnectionPage::setSerialConfig(const QString& portName, int baudRate, const QString& parity, int dataBits,
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

	int index = 1;
	m_ui->serialTable->item(0, index++)->setText(QString::number(baudRate));
	m_ui->serialTable->item(0, index++)->setText(parity);
	m_ui->serialTable->item(0, index++)->setText(QString::number(dataBits));
	m_ui->serialTable->item(0, index++)->setText(stopBits);
}

void ConnectionPage::setNetworkConfig(const QString& ipAddress, int port)
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

void ConnectionPage::setTransportMode(const QString& mode)
{
	if (m_connectionStack == nullptr)
	{
		return;
	}

	const bool serialMode = (mode == QStringLiteral("串口")) || (mode.compare(
		QStringLiteral("serial"), Qt::CaseInsensitive) == 0);
	m_connectionStack->setCurrentIndex(serialMode ? 0 : 1);
}

void ConnectionPage::refreshPorts()
{
	auto serialInfoList = QSerialPortInfo::availablePorts();
	m_portCombo->clear();
	for (auto& serialInfo : serialInfoList)
	{
		m_portCombo->addItem(serialInfo.portName());
	}
}

void ConnectionPage::onCommuteModeChanged(int index)
{
	if (m_connectionStack != nullptr)
	{
		m_connectionStack->setCurrentIndex(index);
	}
	emit transportModeChanged(index == 0 ? QStringLiteral("串口") : QStringLiteral("网口"));
}
