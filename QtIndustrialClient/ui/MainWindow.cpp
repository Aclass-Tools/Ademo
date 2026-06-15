#include "MainWindow.h"
#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QToolBar>

#include "services/ConfigService.h"
#include "services/ProtocolParser.h"
#include "ui/CommandOperationPage.h"
#include "ui/ParamConfigPage.h"
#include "ui_MainWindow.h"
#include "ui/ConnectionPage.h"

namespace
{
	constexpr quint8 kUpperComputerId = 0x00;
}

MainWindow::MainWindow()
	: m_ui(std::make_unique<Ui::MainWindow>())
{
	m_ui->setupUi(this);
	buildUi();

	m_serialService.setLogCallback([this](const QString& message)
	{
		appendLog(message);
	});
	m_serialService.setFrameReceivedCallback([this](const QByteArray& frame)
	{
		appendLog(QStringLiteral("收到协议帧: %1").arg(QString::fromLatin1(frame.toHex(' '))));
		const ProtocolFrame parsed = ProtocolParser::parseFrame(frame);
		if (parsed.isValid && parsed.frameType == 0x54 && m_commandOperationPage != nullptr)
		{
			quint8 protocolType = parsed.protocolType;

			m_commandOperationPage->updateValues(protocolType,
			                                     static_cast<int>(parsed.mainCommand),
			                                     static_cast<int>(parsed.subCommand),
			                                     parsed.payload);

			// 同时更新绘图数据
			updatePlotData(protocolType,
			               static_cast<int>(parsed.mainCommand),
			               static_cast<int>(parsed.subCommand),
			               parsed.payload);
		}
	});
	m_networkService.setLogCallback([this](const QString& message)
	{
		appendLog(message);
	});
	m_networkService.setFrameReceivedCallback([this](const QByteArray& frame)
	{
		appendLog(QStringLiteral("收到网口协议帧: %1").arg(QString::fromLatin1(frame.toHex(' '))));
		const ProtocolFrame parsed = ProtocolParser::parseFrame(frame);
		if (parsed.isValid && parsed.frameType == 0x54 && m_commandOperationPage != nullptr)
		{
			quint8 protocolType = parsed.protocolType;

			m_commandOperationPage->updateValues(protocolType,
			                                     static_cast<int>(parsed.mainCommand),
			                                     static_cast<int>(parsed.subCommand),
			                                     parsed.payload);

			// 同时更新绘图数据
			updatePlotData(protocolType,
			               static_cast<int>(parsed.mainCommand),
			               static_cast<int>(parsed.subCommand),
			               parsed.payload);
		}
	});

	refreshPorts();
	loadDefaultConfig();
	updateConnectionBadge();
	if (m_paramConfigPage != nullptr)
	{
		m_paramConfigPage->showProtocolPage();
	}
	switchPage(0);
}

void MainWindow::updatePlotData(quint8 protocolType, int groupId, int commandId, const QByteArray& payload)
{
	// 只更新A3协议的数据
	if (protocolType != PROTOCOL_A3)
	{
		return;
	}

	// 找到对应的参数记录来获取数据类型信息
	for (const auto& record : m_commandOperationPage->records())
	{
		if (record.protocolType == protocolType &&
			record.groupId == groupId && record.commandId == commandId)
		{
			// 解码数据值为double
			double value = decodeValueToDouble(record, payload, 0);

			// 更新到绘图对话框
			m_commandOperationPage->onNewDataReceived(protocolType, groupId, commandId, value);
			break;
		}
	}
}

double MainWindow::decodeValueToDouble(const ParameterRecord& record, const QByteArray& payload, int offset)
{
	const QString type   = record.dataType.trimmed().toLower();
	const int     length = qMax(0, record.dataLength);

	if (offset < 0 || offset >= payload.size() || length <= 0)
	{
		return 0.0;
	}

	const QByteArray chunk = payload.mid(offset, length);

	// 根据数据类型解码
	if (type == QStringLiteral("uint8_t") && chunk.size() >= 1)
	{
		return static_cast<quint8>(chunk.at(0));
	}
	if (type == QStringLiteral("int8_t") && chunk.size() >= 1)
	{
		return static_cast<qint8>(chunk.at(0));
	}
	if (type == QStringLiteral("uint16_t") && chunk.size() >= 2)
	{
		return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(chunk.constData()));
	}
	if (type == QStringLiteral("int16_t") && chunk.size() >= 2)
	{
		return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(chunk.constData()));
	}
	if (type == QStringLiteral("uint32_t") && chunk.size() >= 4)
	{
		return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(chunk.constData()));
	}
	if (type == QStringLiteral("int32_t") && chunk.size() >= 4)
	{
		return qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(chunk.constData()));
	}
	if (type == QStringLiteral("float") && chunk.size() >= 4)
	{
		const quint32 raw   = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(chunk.constData()));
		float         value = 0.0f;
		std::memcpy(&value, &raw, sizeof(float));
		return value;
	}
	if (type == QStringLiteral("double") && chunk.size() >= 8)
	{
		const quint64 raw   = qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(chunk.constData()));
		double        value = 0.0;
		std::memcpy(&value, &raw, sizeof(double));
		return value;
	}
	if (type == QStringLiteral("int64_t") && chunk.size() >= 8)
	{
		return static_cast<double>(qFromLittleEndian<qint64>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("uint64_t") && chunk.size() >= 8)
	{
		return static_cast<double>(qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(chunk.constData())));
	}

	return 0.0; // 默认值
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
	resize(1680, 960);
	setWindowTitle(QStringLiteral("A类维护_V0.0.8"));

	auto* toolBar = new QToolBar(QStringLiteral("Main"), this);
	toolBar->setMovable(false);
	toolBar->setStyleSheet(QStringLiteral("QToolBar { background: #a9d6d6; spacing: 12px; }"));
	addToolBar(Qt::TopToolBarArea, toolBar);

	const QStringList toolNames = {
		QStringLiteral("参数导入"),
		QStringLiteral("板卡配置"),
		QStringLiteral("指令操作"),
		QStringLiteral("数据绘图"),
		QStringLiteral("设备连接"),
		QStringLiteral("板卡升级")
	};

	auto loadJsonAction = [this]()
	{
		const QString filePath = QFileDialog::getOpenFileName(
			this,
			QStringLiteral("选择参数配置"),
			m_configPath.isEmpty() ? QStringLiteral(".") : QFileInfo(m_configPath).absolutePath(),
			QStringLiteral("JSON Files (*.json)"));
		if (!filePath.isEmpty())
		{
			loadConfigFromFile(filePath);
		}
	};

	for (int i = 0; i < toolNames.size(); ++i)
	{
		auto* button = new QPushButton(toolNames.at(i), this);
		button->setMinimumSize(74, 42);
		button->setStyleSheet(QStringLiteral("background: transparent; border: none; padding: 6px 8px;"));
		if (i == 3)
		{
			button->setEnabled(false);
		}
		if (i == 0)
		{
			QObject::connect(button, &QPushButton::clicked, this, loadJsonAction);
		}
		else
		{
			QObject::connect(button, &QPushButton::clicked, this, [this, i]()
			{
				if (i == 1)
				{
					if (m_paramConfigPage != nullptr)
					{
						m_paramConfigPage->showProtocolPage();
					}
					switchPage(0);
				}
				else if (i == 2)
				{
					switchPage(1);
				}
				else if (i == 4)
				{
					switchPage(2);
					m_connectionPage->setTransportMode(ConfigService::instance().currentConfigure().comminicate.mode);
				}
			});
		}
		toolBar->addWidget(button);
	}

	m_boardTable = m_ui->boardTable;
	m_boardTable->setHorizontalHeaderLabels({QStringLiteral("板卡"), QStringLiteral("操作")});
	m_boardTable->verticalHeader()->setVisible(false);
	m_boardTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_boardTable->setSelectionMode(QAbstractItemView::SingleSelection);
	m_boardTable->setStyleSheet(QStringLiteral(
		"QTableWidget { border: 1px solid #d9e2ec; background: white; }"
		"QHeaderView::section { background: #f3f4f6; padding: 6px; }"));
	m_boardTable->setRowCount(0);
	m_connectButton = nullptr;

	m_paramConfigPage      = new ParamConfigPage(this);
	m_commandOperationPage = new CommandOperationPage(this);
	m_connectionPage       = new ConnectionPage(this);
	m_paramConfigPage->setRefreshPortsCallback([this]()
	{
		refreshPorts();
	});

	m_centerStack = m_ui->centerStack;
	m_centerStack->addWidget(m_paramConfigPage);
	m_centerStack->addWidget(m_commandOperationPage);
	m_centerStack->addWidget(m_connectionPage);

	m_logView = m_ui->logView;
	m_logView->setStyleSheet(QStringLiteral(
		"QTextEdit { background: white; color: #ef4444; border: 1px solid #d9e2ec; padding: 8px; font-family: Consolas; }"));
	statusBar()->showMessage(QStringLiteral("客户端已初始化"));

	m_commandOperationPage->setActionHandler([this](const ParameterRecord& record, ParameterAction action)
	{
		handleParameterAction(record, action);
	});
}

void MainWindow::refreshPorts()
{
	QStringList portNames;
	const auto  ports = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo& portInfo : ports)
	{
		portNames.push_back(portInfo.portName());
	}

	if (m_paramConfigPage != nullptr)
	{
		m_paramConfigPage->refreshPorts(portNames);
	}
	appendLog(QStringLiteral("已刷新串口列表，共 %1 个端口").arg(portNames.size()));
}

void MainWindow::loadDefaultConfig()
{
	loadConfigFromFile(ConfigService::instance().findDefaultConfigPath());
}

void MainWindow::loadConfigFromFile(const QString& filePath)
{
	QVector<ParameterRecord> loadedRecords;
	m_configPath = filePath;
	if (m_paramConfigPage != nullptr)
	{
		if (!m_paramConfigPage->loadConfig(filePath))
		{
			m_paramConfigPage->setRecords({});
			m_commandOperationPage->setRecords({});
			m_boardTable->clearContents();
			m_boardTable->setRowCount(0);
			m_configPath.clear();
			appendLog(QStringLiteral("加载配置文件失败: %1").arg(QDir::toNativeSeparators(filePath)));
			return;
		}
		loadedRecords = m_paramConfigPage->records();
		m_commandOperationPage->setRecords(loadedRecords);
	}
	else
	{
		QString errorMessage;
		loadedRecords = ConfigService::instance().loadRecords(filePath, &errorMessage);
		if (loadedRecords.isEmpty() && !errorMessage.isEmpty())
		{
			m_commandOperationPage->setRecords({});
			m_boardTable->clearContents();
			m_boardTable->setRowCount(0);
			m_configPath.clear();
			appendLog(errorMessage);
			return;
		}
		m_commandOperationPage->setRecords(loadedRecords);
	}
	m_boardTable->clearContents();
	m_boardTable->setRowCount(1);
	auto* fileItem = new QTableWidgetItem(QFileInfo(filePath).baseName());
	fileItem->setTextAlignment(Qt::AlignCenter);
	m_boardTable->setItem(0, 0, fileItem);

	auto* connectButton = new QPushButton(
		(m_serialService.isOpen() || m_networkService.isConnected()) ? QStringLiteral("断开") : QStringLiteral("连接"),
		m_boardTable);
	QObject::connect(connectButton, &QPushButton::clicked, this, [this]()
	{
		toggleConnection();
	});
	m_boardTable->setCellWidget(0, 1, connectButton);
	m_connectButton = connectButton;

	statusBar()->showMessage(QStringLiteral("配置已加载: %1 项").arg(loadedRecords.size()));
	appendLog(QStringLiteral("已加载配置文件: %1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::switchPage(int pageIndex)
{
	if (m_centerStack != nullptr && pageIndex >= 0 && pageIndex < m_centerStack->count())
	{
		if (m_centerStack->currentIndex() == 0 && pageIndex != 0 &&
			m_paramConfigPage != nullptr && !m_paramConfigPage->validateBeforeLeave(true))
		{
			return;
		}
		m_centerStack->setCurrentIndex(pageIndex);
	}
}

void MainWindow::toggleConnection()
{
	if (m_activeConnectionMode == ActiveConnectionMode::Serial && m_serialService.isOpen())
	{
		m_serialService.closePort();
		m_activeConnectionMode = ActiveConnectionMode::None;
		updateConnectionBadge();
		return;
	}

	if (m_activeConnectionMode == ActiveConnectionMode::Network && m_networkService.isConnected())
	{
		m_networkService.disconnectFromHost();
		m_activeConnectionMode = ActiveConnectionMode::None;
		updateConnectionBadge();
		return;
	}

	if (m_paramConfigPage != nullptr && m_paramConfigPage->currentTransportMode() == QStringLiteral("串口"))
	{
		const QString portName = m_connectionPage->selectedPortName();
		if (portName.isEmpty())
		{
			QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择可用串口。"));
			return;
		}

		QString errorMessage;
		if (!m_serialService.openPort(portName, m_connectionPage->selectedBaudRate(), &errorMessage))
		{
			QMessageBox::warning(this, QStringLiteral("连接失败"), errorMessage);
			return;
		}

		m_activeConnectionMode = ActiveConnectionMode::Serial;
		updateConnectionBadge();
		return;
	}

	const QString ipAddress = m_connectionPage == nullptr
	                          ? QString()
	                          : m_connectionPage->selectedIpAddress();
	const int networkPort = m_connectionPage == nullptr ? 0 : m_connectionPage->selectedNetworkPort();
	if (ipAddress.isEmpty() || networkPort <= 0)
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先填写有效的网口连接配置。"));
		return;
	}

	m_networkService.connectToHost(ipAddress, static_cast<quint16>(networkPort));
	if (!m_networkService.isConnected())
	{
		QMessageBox::warning(this, QStringLiteral("连接失败"), QStringLiteral("网口连接失败，请检查 IP 地址和端口。"));
		updateConnectionBadge();
		return;
	}

	m_activeConnectionMode = ActiveConnectionMode::Network;
	updateConnectionBadge();
}

void MainWindow::handleParameterAction(const ParameterRecord& record, ParameterAction action)
{
	const bool       isWrite = action == ParameterAction::Write;
	const QByteArray payload = isWrite ? ProtocolParser::encodeWritePayload(record) : QByteArray();
	const quint8     boardId = static_cast<quint8>(
		m_paramConfigPage == nullptr ? 1 : m_paramConfigPage->boardId());
	const quint8     protocolType = record.protocolType;
	const QByteArray frame        = isWrite
	                         ? ProtocolParser::buildWriteFrame(
		                         protocolType,
		                         kUpperComputerId,
		                         boardId,
		                         static_cast<quint8>(record.groupId),
		                         static_cast<quint8>(record.commandId),
		                         payload)
	                         : ProtocolParser::buildReadFrame(
		                         protocolType,
		                         kUpperComputerId,
		                         boardId,
		                         static_cast<quint8>(record.groupId),
		                         static_cast<quint8>(record.commandId));

	const QString actionLabel = isWrite ? QStringLiteral("写入") : QStringLiteral("读取");
	appendLog(QStringLiteral("%1 参数: [%2] %3 / 说明: %4")
		.arg(actionLabel, record.groupName, record.parameterName, record.description));

	if (m_paramConfigPage != nullptr && m_paramConfigPage->currentTransportMode() == QStringLiteral("网口"))
	{
		QString errorMessage;
		if (!m_networkService.sendFrame(frame, &errorMessage))
		{
			QMessageBox::warning(this, QStringLiteral("发送失败"), errorMessage);
			return;
		}

		statusBar()->showMessage(QStringLiteral("%1 指令已发送: %2").arg(actionLabel, record.commandName), 4000);
		return;
	}

	if (!m_serialService.isOpen())
	{
		appendLog(QStringLiteral("串口未连接，已生成示例报文: %1").arg(QString::fromLatin1(frame.toHex(' '))));
		statusBar()->showMessage(QStringLiteral("%1 已准备，等待连接串口").arg(actionLabel), 4000);
		return;
	}

	QString errorMessage;
	if (!m_serialService.sendFrame(frame, &errorMessage))
	{
		QMessageBox::warning(this, QStringLiteral("发送失败"), errorMessage);
		return;
	}

	statusBar()->showMessage(QStringLiteral("%1 指令已发送: %2").arg(actionLabel, record.commandName), 4000);
}

void MainWindow::appendLog(const QString& message)
{
	const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
	m_logView->append(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void MainWindow::updateConnectionBadge()
{
	if (m_activeConnectionMode == ActiveConnectionMode::Serial && m_serialService.isOpen())
	{
		if (m_connectButton != nullptr)
		{
			m_connectButton->setText(QStringLiteral("断开"));
		}
		statusBar()->showMessage(QStringLiteral("串口已连接"), 3000);
		return;
	}

	if (m_activeConnectionMode == ActiveConnectionMode::Network && m_networkService.isConnected())
	{
		if (m_connectButton != nullptr)
		{
			m_connectButton->setText(QStringLiteral("断开"));
		}
		statusBar()->showMessage(QStringLiteral("网口已连接"), 3000);
		return;
	}

	if (m_connectButton != nullptr)
	{
		m_connectButton->setText(QStringLiteral("连接"));
	}
	statusBar()->showMessage(QStringLiteral("未连接"), 3000);
}
