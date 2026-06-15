#include "ui/ParamConfigPage.h"
#include "ui_ParamConfigPage.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <utility>

#include "services/ConfigService.h"
#include "ui/ConnectionConfigPage.h"
#include "ui/ProtocolConfigPage.h"

ParamConfigPage::ParamConfigPage(QWidget* parent)
	: QWidget(parent)
	  , m_protocolPage(new ProtocolConfigPage(this))
	  , m_connectionPage(new ConnectionConfigPage(this))
	  , m_ui(std::make_unique<Ui::ParamConfigPageClass>())
{
	m_ui->setupUi(this);

	m_ui->contentStackedWidget->addTab(m_protocolPage, QStringLiteral("协议配置"));
	m_ui->contentStackedWidget->addTab(m_connectionPage, QStringLiteral("网络配置"));
	m_ui->contentStackedWidget->setCurrentWidget(m_protocolPage);
	m_lastTabIndex = m_ui->contentStackedWidget->currentIndex();

	connect(m_ui->saveButton, &QPushButton::clicked, this, &ParamConfigPage::on_saveButton_clicked);
	connect(m_ui->restoreButton, &QPushButton::clicked, this, &ParamConfigPage::on_restoreButton_clicked);
	connect(m_ui->contentStackedWidget, &QTabWidget::currentChanged, this, &ParamConfigPage::onContentTabChanged);

	connect(m_protocolPage, &ProtocolConfigPage::saveSuccess, this, &ParamConfigPage::saveSuccess);
	connect(m_protocolPage, &ProtocolConfigPage::saveFailed, this, &ParamConfigPage::saveFailed);
	connect(m_connectionPage, &ConnectionConfigPage::transportModeChanged, this, &ParamConfigPage::transportModeChanged);
}

ParamConfigPage::~ParamConfigPage() = default;

void ParamConfigPage::setRecords(const QVector<ParameterRecord>& records)
{
	if (m_protocolPage != nullptr)
	{
		m_protocolPage->setRecords(records);
	}
}

QVector<ParameterRecord> ParamConfigPage::records() const
{
	return m_protocolPage == nullptr ? QVector<ParameterRecord>() : m_protocolPage->records();
}

bool ParamConfigPage::checkForDuplicateSubCommands()
{
	return m_protocolPage == nullptr ? true : m_protocolPage->checkForDuplicateSubCommands();
}

QString ParamConfigPage::selectedPortName() const
{
	return m_connectionPage == nullptr ? QString() : m_connectionPage->selectedPortName();
}

int ParamConfigPage::selectedBaudRate() const
{
	return m_connectionPage == nullptr ? 115200 : m_connectionPage->selectedBaudRate();
}

QString ParamConfigPage::selectedIpAddress() const
{
	return m_connectionPage == nullptr ? QString() : m_connectionPage->selectedIpAddress();
}

int ParamConfigPage::selectedNetworkPort() const
{
	return m_connectionPage == nullptr ? 0 : m_connectionPage->selectedNetworkPort();
}

QString ParamConfigPage::currentTransportMode() const
{
	return m_connectionPage == nullptr ? QStringLiteral("串口") : m_connectionPage->currentTransportMode();
}

int ParamConfigPage::boardId() const
{
	return m_connectionPage == nullptr ? 1 : m_connectionPage->boardId();
}

int ParamConfigPage::parentId() const
{
	return 0;
}

void ParamConfigPage::refreshPorts(const QStringList& ports)
{
	if (m_connectionPage != nullptr)
	{
		m_connectionPage->refreshPorts(ports);
	}
}

void ParamConfigPage::setRefreshPortsCallback(std::function<void()> callback)
{
	if (m_connectionPage != nullptr)
	{
		m_connectionPage->setRefreshPortsCallback(std::move(callback));
	}
}

void ParamConfigPage::showProtocolPage()
{
	if (m_ui->contentStackedWidget != nullptr && m_protocolPage != nullptr)
	{
		m_ui->contentStackedWidget->setCurrentWidget(m_protocolPage);
	}
}

void ParamConfigPage::showConnectionPage()
{
	if (m_ui->contentStackedWidget != nullptr && m_connectionPage != nullptr)
	{
		m_ui->contentStackedWidget->setCurrentWidget(m_connectionPage);
	}
}

bool ParamConfigPage::validateBeforeLeave(bool showMessage) const
{
	if (m_connectionPage == nullptr || m_ui->contentStackedWidget == nullptr)
	{
		return true;
	}
	if (m_ui->contentStackedWidget->currentWidget() != m_connectionPage)
	{
		return true;
	}
	if (m_connectionPage->currentTransportMode() != QStringLiteral("网口"))
	{
		return true;
	}

	QString errorMessage;
	const bool valid = m_connectionPage->validateNetworkAddress(&errorMessage);
	if (!valid && showMessage)
	{
		QMessageBox::warning(const_cast<ParamConfigPage*>(this), QStringLiteral("网址错误"),
		                     errorMessage.isEmpty() ? QStringLiteral("网址编写错误。") : errorMessage);
	}
	return valid;
}

bool ParamConfigPage::saveConfig(const QString& filePath)
{
	if (m_protocolPage == nullptr || m_connectionPage == nullptr)
	{
		return false;
	}

	// 先把连接配置写入全局配置快照，再由协议页统一保存
	ParamConfigure configure = ConfigService::instance().currentConfigure();
	configure.comminicate = serializeConnectionConfig();
	ConfigService::instance().setCurrentConfigure(configure);
	return m_protocolPage->saveConfig(filePath);
}

bool ParamConfigPage::loadConfig(const QString& filePath)
{
	if (m_protocolPage == nullptr || m_connectionPage == nullptr)
	{
		return false;
	}

	if (!m_protocolPage->loadConfig(filePath))
	{
		return false;
	}

	deserializeConnectionConfig(ConfigService::instance().currentConfigure().comminicate);
	return true;
}

void ParamConfigPage::on_saveButton_clicked()
{
	const QString filePath = QFileDialog::getSaveFileName(
		this,
		tr("保存配置"),
		ConfigService::instance().findDefaultConfigPath(),
		tr("JSON 文件 (*.json);;所有文件 (*.*)"));
	if (!filePath.isEmpty())
	{
		saveConfig(filePath);
	}
}

void ParamConfigPage::on_restoreButton_clicked()
{
	const QString filePath = QFileDialog::getOpenFileName(
		this,
		tr("加载配置"),
		ConfigService::instance().findDefaultConfigPath(),
		tr("JSON 文件 (*.json);;所有文件 (*.*)"));
	if (!filePath.isEmpty())
	{
		loadConfig(filePath);
	}
}

ParamComminicate ParamConfigPage::serializeConnectionConfig() const
{
	ParamComminicate comminicate = ConfigService::instance().defaultComminicate();
	comminicate.mode             = currentTransportMode();
	comminicate.serial.port      = selectedPortName();
	comminicate.serial.baudRate  = static_cast<quint64>(selectedBaudRate());
	comminicate.serial.parity    = m_connectionPage == nullptr ? QStringLiteral("NoParity") : m_connectionPage->selectedParity();
	comminicate.serial.dataBits  = static_cast<quint64>(m_connectionPage == nullptr ? 8 : m_connectionPage->selectedDataBits());
	comminicate.serial.stopBits  = m_connectionPage == nullptr ? QStringLiteral("OneStop") : m_connectionPage->selectedStopBits();
	comminicate.serial.boardId   = boardId();
	comminicate.serial.parentId  = parentId();
	comminicate.network.ip       = selectedIpAddress();
	comminicate.network.port     = static_cast<quint64>(selectedNetworkPort());
	return comminicate;
}

void ParamConfigPage::deserializeConnectionConfig(const ParamComminicate& comminicate)
{
	if (m_connectionPage == nullptr)
	{
		return;
	}

	m_connectionPage->setSerialConfig(
		comminicate.serial.port,
		static_cast<int>(comminicate.serial.baudRate),
		comminicate.serial.parity,
		static_cast<int>(comminicate.serial.dataBits),
		comminicate.serial.stopBits);

	m_connectionPage->setNetworkConfig(
		comminicate.network.ip,
		static_cast<int>(comminicate.network.port));

	m_connectionPage->setTransportMode(comminicate.mode);
}

void ParamConfigPage::onContentTabChanged(int index)
{
	if (m_switchingTab || m_ui->contentStackedWidget == nullptr)
	{
		return;
	}

	const QWidget* previous = m_ui->contentStackedWidget->widget(m_lastTabIndex);
	if (previous == m_connectionPage && index != m_lastTabIndex && !validateBeforeLeave(true))
	{
		m_switchingTab = true;
		m_ui->contentStackedWidget->setCurrentIndex(m_lastTabIndex);
		m_switchingTab = false;
		return;
	}

	m_lastTabIndex = index;
}
