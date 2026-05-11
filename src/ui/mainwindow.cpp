// MainWindow 实现
// ----------------
// 本文件负责装配顶层桌面外壳，并连接：
// - 导航按钮 -> 堆叠页切换
// - 基础设施客户端事件 -> 状态栏显示
//
// 页面内部细节已解耦到 src/ui/pages/*。

#include "mainwindow.h"

#include "network/apiclient.h"
#include "network/websocketclient.h"
#include "ui/pages/homepage.h"
#include "ui/pages/protocoleditorpage.h"
#include "ui/pages/protocoldebugpage.h"
#include "ui/pages/protocolexportpage.h"
#include "ui/pages/deviceupgradepage.h"
#include "ui/pages/terminalpage.h"
#include "ui/pages/logpage.h"
#include "ui/pages/pluginpage.h"
#include "ui/theme/thememanager.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QApplication>
#include <QStatusBar>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 从 Designer UI 构建静态控件树。
    ui->setupUi(this);
    // 再次覆盖窗口图标，防止某些平台未继承 QApplication 级别图标。
    setWindowIcon(QApplication::windowIcon());
    m_palette = ThemeManager::palette(m_themeKind);

    // 基础传输客户端。
    m_apiClient = new ApiClient(this);
    m_wsClient = new WebSocketClient(this);

    // 解耦页面实例：每个页面都有独立 .ui 与类。
    m_homePage = new HomePage(this);
    m_protocolEditorPage = new ProtocolEditorPage(this);
    m_protocolDebugPage = new ProtocolDebugPage(this);
    m_protocolExportPage = new ProtocolExportPage(this);
    m_deviceUpgradePage = new DeviceUpgradePage(this);
    m_terminalPage = new TerminalPage(this);
    m_logPage = new LogPage(this);
    m_pluginPage = new PluginPage(this);

    // 将页面注入 mainwindow.ui 中预留的容器布局。
    ui->verticalLayoutHome->addWidget(m_homePage);
    ui->verticalLayoutProtocol->addWidget(m_protocolEditorPage);
    ui->verticalLayoutProtocolDebug->addWidget(m_protocolDebugPage);
    ui->verticalLayoutProtocolExport->addWidget(m_protocolExportPage);
    ui->verticalLayoutDeviceUpgrade->addWidget(m_deviceUpgradePage);
    ui->verticalLayoutTerminal->addWidget(m_terminalPage);
    ui->verticalLayoutLog->addWidget(m_logPage);
    ui->verticalLayoutPlugin->addWidget(m_pluginPage);

    // 主窗体基础样式由主题管理器统一生成。
    setStyleSheet(ThemeManager::mainWindowStyle(m_palette));

    // 顶部两行方块导航按钮统一样式。
    auto setNavButtonStyle = [this](QToolButton *button) {
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setStyleSheet(ThemeManager::navButtonStyle(m_palette));
    };
    setNavButtonStyle(ui->btnToolHome);
    setNavButtonStyle(ui->btnProtocolEdit);
    setNavButtonStyle(ui->btnDeviceConnect);
    setNavButtonStyle(ui->btnProtocolDebug);
    setNavButtonStyle(ui->btnDeviceUpgrade);
    setNavButtonStyle(ui->btnTerminalDebug);
    setNavButtonStyle(ui->btnLogExport);
    setNavButtonStyle(ui->btnPluginScript);

    // 强制设置导航文本，避免 .ui 在不同编码环境下出现中文乱码。
    // ui->btnToolHome->setText(QStringLiteral("工具\n首页"));
    // ui->btnProtocolEdit->setText(QStringLiteral("协议\n编辑"));
    // ui->btnDeviceConnect->setText(QStringLiteral("设备\n连接"));
    // ui->btnProtocolDebug->setText(QStringLiteral("协议\n调试"));
    // ui->btnDeviceUpgrade->setText(QStringLiteral("设备\n升级"));
    // ui->btnTerminalDebug->setText(QStringLiteral("终端\n调试"));
    // ui->btnLogExport->setText(QStringLiteral("日志\n导出"));
    // ui->btnPluginScript->setText(QStringLiteral("外挂\n脚本"));

    // 默认选中首页。
    ui->btnToolHome->setChecked(true);
    ui->mainPageStack->setCurrentWidget(ui->pageHome);

    setupNavigationTree();
    setupConnections();

    // 启动探针。
    m_apiClient->getVersion();
    m_wsClient->connectToServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNavigationTree()
{
    // 底部状态栏轻量指标。
    m_connectionLabel = new QLabel(QStringLiteral("WS: Connecting"), this);
    m_userLabel = new QLabel(QStringLiteral("User: engineer"), this);
    m_configVersionLabel = new QLabel(QStringLiteral("Config Version: --"), this);
    statusBar()->addWidget(m_connectionLabel);
    statusBar()->addPermanentWidget(m_userLabel);
    statusBar()->addPermanentWidget(m_configVersionLabel);
}

void MainWindow::setupConnections()
{
    // 说明：
    // 当前导航使用显式互斥勾选，行为最直观。
    // 未来导航数量继续增加时可切到 QActionGroup/QButtonGroup。

    connect(ui->btnToolHome, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(true);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageHome);
    });
    connect(ui->btnProtocolEdit, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(true);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageProtocolEditor);
    });
    connect(ui->btnDeviceConnect, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(true);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageProtocolExport);
    });
    connect(ui->btnProtocolDebug, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(true);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageProtocolDebug);
    });
    connect(ui->btnDeviceUpgrade, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(true);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageDeviceUpgrade);
    });
    connect(ui->btnTerminalDebug, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(true);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageTerminal);
    });
    connect(ui->btnLogExport, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(true);
        ui->btnPluginScript->setChecked(false);
        ui->mainPageStack->setCurrentWidget(ui->pageLog);
    });
    connect(ui->btnPluginScript, &QToolButton::clicked, this, [this]() {
        ui->btnToolHome->setChecked(false);
        ui->btnProtocolEdit->setChecked(false);
        ui->btnDeviceConnect->setChecked(false);
        ui->btnProtocolDebug->setChecked(false);
        ui->btnDeviceUpgrade->setChecked(false);
        ui->btnTerminalDebug->setChecked(false);
        ui->btnLogExport->setChecked(false);
        ui->btnPluginScript->setChecked(true);
        ui->mainPageStack->setCurrentWidget(ui->pagePlugin);
    });

    // 基础设施事件 -> 状态指示。
    connect(m_apiClient, &ApiClient::versionReceived, this, [this](const QString &version) {
        m_configVersionLabel->setText(QStringLiteral("Config Version: %1").arg(version.isEmpty() ? QStringLiteral("--") : version));
    });
    // 占位阶段：先保留事件连接，后续由页面层接入实际逻辑。
    connect(m_apiClient, &ApiClient::pointsReceived, this, [](const QByteArray &) {});
    connect(m_apiClient, &ApiClient::conflictDetected, this, [](int) {});
    connect(m_apiClient, &ApiClient::requestFailed, this, [this](const QString &message) {
        showStatus(QStringLiteral("Request failed: %1").arg(message));
    });

    connect(m_wsClient, &WebSocketClient::connected, this, [this]() {
        m_connectionLabel->setText(QStringLiteral("WS: Online"));
        showStatus(QStringLiteral("WebSocket connected"));
    });
    connect(m_wsClient, &WebSocketClient::disconnected, this, [this]() {
        m_connectionLabel->setText(QStringLiteral("WS: Offline"));
        showStatus(QStringLiteral("WebSocket disconnected"));
    });
    connect(m_wsClient, &WebSocketClient::textMessageReceived, this, [](const QString &) {});
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, [this](const QString &message) {
        showStatus(QStringLiteral("WebSocket error: %1").arg(message));
    });
}

void MainWindow::showStatus(const QString &message)
{
    // 临时提示条；长期状态由状态栏标签承载。
    statusBar()->showMessage(message, 3000);
}

