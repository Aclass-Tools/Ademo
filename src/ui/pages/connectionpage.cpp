#include "connectionpage.h"
#include "ui_connectionpage.h"
#include "services/connectionmanager.h"
#include "ui/theme/thememanager.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStackedWidget>

ConnectionPage::ConnectionPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ConnectionPage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    initCombos();

    // 传输方式切换 → 切换配置面板。
    ui->modeCombo->addItem(QStringLiteral("串口"), int(0));
    ui->modeCombo->addItem(QStringLiteral("网口"), int(1));
    connect(ui->modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionPage::onModeChanged);

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &ConnectionPage::refreshPorts);
    connect(ui->connectButton, &QPushButton::clicked, this, &ConnectionPage::onConnectClicked);

    // 初始进入串口模式。
    ui->configStack->setCurrentIndex(0);
    refreshPorts();
}

ConnectionPage::~ConnectionPage()
{
    delete ui;
}

void ConnectionPage::setConnectionManager(services::ConnectionManager *mgr)
{
    m_conn = mgr;
    if (m_conn) {
        connect(m_conn, &services::ConnectionManager::connectionChanged,
                this, [this](bool connected, services::TransportMode) {
                    onConnectionChanged(connected);
                });
        connect(m_conn, &services::ConnectionManager::log, this, [this](const QString &msg) {
            ui->statusLabel->setText(QStringLiteral("状态：%1").arg(msg));
        });
        onConnectionChanged(m_conn->isConnected());
    }
}

void ConnectionPage::onPageActivated()
{
    // 进入页面时刷新端口列表 + 同步连接状态。
    refreshPorts();
    if (m_conn) {
        onConnectionChanged(m_conn->isConnected());
    }
}

void ConnectionPage::initCombos()
{
    // 波特率。
    ui->baudCombo->addItems({QStringLiteral("1200"), QStringLiteral("2400"), QStringLiteral("4800"),
                             QStringLiteral("9600"), QStringLiteral("19200"), QStringLiteral("38400"),
                             QStringLiteral("57600"), QStringLiteral("115200"), QStringLiteral("230400"),
                             QStringLiteral("460800"), QStringLiteral("921600")});
    ui->baudCombo->setCurrentText(QStringLiteral("115200"));

    // 数据位。
    ui->dataBitsCombo->addItems({QStringLiteral("8"), QStringLiteral("7"), QStringLiteral("6"), QStringLiteral("5")});

    // 校验位（索引与 SerialPortManager 约定一致：0无 1偶 2奇 3空号 4标记）。
    ui->parityCombo->addItems({QStringLiteral("无"), QStringLiteral("偶"), QStringLiteral("奇"),
                               QStringLiteral("空号"), QStringLiteral("标记")});

    // 停止位（0:1 1:1.5 2:2）。
    ui->stopBitsCombo->addItems({QStringLiteral("1"), QStringLiteral("1.5"), QStringLiteral("2")});

    // 流控（0无 1RTS/CTS 2XON/XOFF 3全部）。
    ui->flowCombo->addItems({QStringLiteral("无"), QStringLiteral("RTS/CTS"), QStringLiteral("XON/XOFF"),
                             QStringLiteral("全部")});
}

void ConnectionPage::onModeChanged(int index)
{
    ui->configStack->setCurrentIndex(index);
    // 网口模式不显示"刷新端口"。
    ui->refreshPortsButton->setVisible(index == 0);
}

void ConnectionPage::refreshPorts()
{
    ui->portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &p : ports) {
        ui->portCombo->addItem(p.portName());
    }
    if (ui->portCombo->count() == 0) {
        ui->portCombo->addItem(QStringLiteral("（无可用串口）"));
    }
}

void ConnectionPage::onConnectClicked()
{
    if (!m_conn) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("连接管理器未初始化。"));
        return;
    }

    // 已连接 → 断开。
    if (m_conn->isConnected()) {
        m_disconnecting = true;
        m_conn->disconnect();
        return;
    }

    const int mode = ui->configStack->currentIndex();
    QString error;
    bool ok = false;
    if (mode == 0) {
        // 串口。
        const QString portName = ui->portCombo->currentText();
        if (portName.isEmpty() || portName.startsWith(QStringLiteral("（"))) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择可用串口。"));
            return;
        }
        services::SerialConfig cfg;
        cfg.portName = portName;
        cfg.baudRate = ui->baudCombo->currentText().toInt();
        cfg.dataBits = ui->dataBitsCombo->currentText().toInt();
        cfg.parityIndex = ui->parityCombo->currentIndex();
        cfg.stopIndex = ui->stopBitsCombo->currentIndex();
        cfg.flowIndex = ui->flowCombo->currentIndex();
        ok = m_conn->connectSerial(cfg, &error);
    } else {
        // 网口。
        services::NetworkConfig cfg;
        cfg.host = ui->ipEdit->text().trimmed();
        cfg.port = quint16(ui->portNetEdit->text().toUShort());
        if (cfg.host.isEmpty() || cfg.port == 0) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请填写有效的 IP 和端口。"));
            return;
        }
        m_conn->connectNetwork(cfg, &error);
        // TCP 异步，这里认为发起成功（真正状态由信号回调更新）。
        ok = true;
    }

    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("连接失败"), error);
    }
}

void ConnectionPage::onConnectionChanged(bool connected)
{
    ui->connectButton->setText(connected ? QStringLiteral("断开") : QStringLiteral("连接"));
    if (connected) {
        ui->statusLabel->setText(QStringLiteral("状态：已连接"));
    } else {
        ui->statusLabel->setText(m_disconnecting ? QStringLiteral("状态：已断开") : QStringLiteral("状态：未连接"));
        m_disconnecting = false;
    }
}
