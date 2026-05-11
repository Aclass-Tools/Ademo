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

#include <QApplication>
#include <QButtonGroup>
#include <QLabel>
#include <QStatusBar>
#include <QToolButton>
#include <QWidget>

#include <type_traits>
#include <utility>

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

    setupPages();

    // 主窗体基础样式由主题管理器统一生成。
    setStyleSheet(ThemeManager::mainWindowStyle(m_palette));

    m_currentProjectName = m_homePage ? m_homePage->currentProjectName() : QString();
    m_currentConfigVersion.clear();
    setupStatusBarContent();
    setupCommunicationBindings();
    switchToPage(ui->pageHome);

    // 启动探针。
    m_apiClient->getVersion();
    m_wsClient->connectToServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPages()
{
    m_navEntries.clear();

    // 创建按钮分组并设置互斥：同一时刻仅允许一个导航按钮处于选中态。
    if (!m_navButtonGroup) {
        m_navButtonGroup = new QButtonGroup(this);
        m_navButtonGroup->setExclusive(true);
    }

    // 统一导航按钮样式，避免每个按钮重复生成样式字符串。
    const QString navStyle = ThemeManager::navButtonStyle(m_palette);

    auto createAttachAndRegister = [this, navStyle](auto *&member, auto *layout, QToolButton *navButton, QWidget *stackPage) {
        using PageT = std::remove_pointer_t<std::remove_reference_t<decltype(member)>>;
        member = new PageT(this);
        if (layout) {
            layout->addWidget(member);
        }

        if (navButton && stackPage) {
            // 初始化导航按钮行为与外观。
            navButton->setCheckable(true);
            navButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
            navButton->setStyleSheet(navStyle);
            m_navButtonGroup->addButton(navButton);

            // 记录按钮与页面的映射，用于统一切页与选中态同步。
            m_navEntries.push_back({navButton, stackPage});

            // 绑定导航点击：点击后切换到对应页面。
            connect(navButton, &QToolButton::clicked, this, [this, page = stackPage]() {
                switchToPage(page);
            });
        }
    };

    createAttachAndRegister(m_homePage, ui->homeLayout, ui->btnToolHome, ui->pageHome);
    createAttachAndRegister(m_protocolEditorPage, ui->protocolEditorLayout, ui->btnProtocolEdit, ui->pageProtocolEditor);
    createAttachAndRegister(m_protocolDebugPage, ui->protocolDebugLayout, ui->btnProtocolDebug, ui->pageProtocolDebug);
    createAttachAndRegister(m_protocolExportPage, ui->protocolExportLayout, ui->btnDeviceConnect, ui->pageProtocolExport);
    createAttachAndRegister(m_deviceUpgradePage, ui->deviceUpgradeLayout, ui->btnDeviceUpgrade, ui->pageDeviceUpgrade);
    createAttachAndRegister(m_terminalPage, ui->terminalLayout, ui->btnTerminalDebug, ui->pageTerminal);
    createAttachAndRegister(m_logPage, ui->logLayout, ui->btnLogExport, ui->pageLog);
    createAttachAndRegister(m_pluginPage, ui->pluginLayout, ui->btnPluginScript, ui->pagePlugin);
}

void MainWindow::setupStatusBarContent()
{
    // 延迟创建：首次调用时创建底栏标签并挂到状态栏。
    if (!m_connectionLabel) {
        m_connectionLabel = new QLabel(this);
        statusBar()->addWidget(m_connectionLabel);
    }
    if (!m_userLabel) {
        m_userLabel = new QLabel(this);
        statusBar()->addPermanentWidget(m_userLabel);
    }
    if (!m_configVersionLabel) {
        m_configVersionLabel = new QLabel(this);
        statusBar()->addPermanentWidget(m_configVersionLabel);
    }

    const QString shownProject = m_currentProjectName.trimmed().isEmpty()
        ? QStringLiteral("未选择项目")
        : m_currentProjectName.trimmed();
    m_connectionLabel->setText(QStringLiteral("当前项目: %1").arg(shownProject));

    const QString shownUser = m_currentUserName.trimmed().isEmpty()
        ? QStringLiteral("--")
        : m_currentUserName.trimmed();
    m_userLabel->setText(QStringLiteral("User: %1").arg(shownUser));

    const QString shownVersion = m_currentConfigVersion.trimmed().isEmpty()
        ? QStringLiteral("--")
        : m_currentConfigVersion.trimmed();
    m_configVersionLabel->setText(QStringLiteral("Config Version: %1").arg(shownVersion));
}

void MainWindow::switchToPage(QWidget *page)
{
    // 防御：空页面直接忽略，避免非法切换。
    if (!page) {
        return;
    }

    // 1) 切换堆叠容器到目标页面。
    ui->mainPageStack->setCurrentWidget(page);

    // 2) 同步所有导航按钮的选中态，保证 UI 状态与当前页面一致。
    for (const NavEntry &entry : std::as_const(m_navEntries)) {
        if (entry.button) {
            entry.button->setChecked(entry.page == page);
        }
    }
}

void MainWindow::setupCommunicationBindings()
{
    // 目标：
    // 1) 将通信层（HTTP/WebSocket）异步事件统一接入主窗口；
    // 2) 在主窗口消费通信异常与关键数据回调；
    // 3) 保留消息分发占位，后续由页面或消息总线承接业务逻辑。

    // ApiClient::versionReceived:
    // 启动后会请求配置版本。收到结果后更新状态栏右侧版本标识，
    // 让用户可直观看到当前后端配置版本是否已成功返回。
    connect(m_apiClient, &ApiClient::versionReceived, this, [this](const QString &version) {
        m_currentConfigVersion = version;
        setupStatusBarContent();
    });

    // ApiClient::pointsReceived / conflictDetected:
    // 当前阶段先占位接住信号，避免后续重构时遗漏连接点；
    // 当页面模型接入后，可将这里改为转发到具体页面或数据总线。
    connect(m_apiClient, &ApiClient::pointsReceived, this, [](const QByteArray &) {});
    connect(m_apiClient, &ApiClient::conflictDetected, this, [](int) {});

    // ApiClient::requestFailed:
    // HTTP/接口请求失败时，在状态栏给出短提示，便于快速定位通信异常。
    connect(m_apiClient, &ApiClient::requestFailed, this, [this](const QString &message) {
        showStatus(QStringLiteral("Request failed: %1").arg(message));
    });

    // WebSocketClient::connected / disconnected:
    // 当前不映射到底栏，只保留短提示用于调试观察连接变化。
    connect(m_wsClient, &WebSocketClient::connected, this, [this]() {
        showStatus(QStringLiteral("WebSocket connected"));
    });
    connect(m_wsClient, &WebSocketClient::disconnected, this, [this]() {
        showStatus(QStringLiteral("WebSocket disconnected"));
    });

    // WebSocketClient::textMessageReceived:
    // 当前先占位；后续可在这里接入消息路由（例如转发给 Home/QML 或各业务页）。
    connect(m_wsClient, &WebSocketClient::textMessageReceived, this, [](const QString &) {});

    // WebSocketClient::errorOccurred:
    // 连接或收发异常时输出错误提示，辅助排障。
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, [this](const QString &message) {
        showStatus(QStringLiteral("WebSocket error: %1").arg(message));
    });

    // 首页项目选择变化 -> 底栏“当前项目”。
    if (m_homePage) {
        connect(m_homePage, &HomePage::currentProjectChanged, this, [this](const QString &projectName) {
            m_currentProjectName = projectName;
            setupStatusBarContent();
        });
    }
}

void MainWindow::showStatus(const QString &message)
{
    // 临时提示条；长期状态由状态栏标签承载。
    statusBar()->showMessage(message, 3000);
}
