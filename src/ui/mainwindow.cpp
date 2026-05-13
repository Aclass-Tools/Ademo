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

    // 先装配页面与导航，再做状态栏绑定，保证后续回调对象都已就绪。
    setupPages();

    // 主窗体基础样式由主题管理器统一生成。
    setStyleSheet(ThemeManager::mainWindowStyle(m_palette));

    // 初始化状态栏文案源，统一通过 setupStatusBarContent() 渲染到底栏。
    // 当前项目由 HomePage 导入后发出的 currentProjectChanged 信号维护。
    m_currentProjectName.clear();
    m_currentConfigVersion.clear();
    setupStatusBarContent();
    setupHomePageBindings();
    ui->btnToolHome->setChecked(true);
    switchToPage(ui->pageHome);

    // 当前阶段主界面不绑定通信层回调；首页数据读取由 HomePage 内部完成。
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPages()
{
    // 创建按钮分组并设置互斥：同一时刻仅允许一个导航按钮处于选中态。
    if (!m_navButtonGroup) {
        m_navButtonGroup = new QButtonGroup(this);
        m_navButtonGroup->setExclusive(true);
    }

    // 统一导航按钮样式，避免每个按钮重复生成样式字符串。
    const QString navStyle = ThemeManager::navButtonStyle(m_palette);

    // 统一页面装配器：
    // - 创建页面对象
    // - 挂到预留布局
    // - 初始化对应导航按钮
    // - 绑定点击切页
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

    // 当前项目：空值回落到“未选择项目”。
    const QString shownProject = m_currentProjectName.trimmed().isEmpty()
        ? QStringLiteral("未选择项目")
        : m_currentProjectName.trimmed();
    m_connectionLabel->setText(QStringLiteral("当前项目: %1").arg(shownProject));

    // 当前用户：空值回落到“--”。
    const QString shownUser = m_currentUserName.trimmed().isEmpty()
        ? QStringLiteral("--")
        : m_currentUserName.trimmed();
    m_userLabel->setText(QStringLiteral("User: %1").arg(shownUser));

    // 配置版本：空值回落到“--”。
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

    // 仅负责切换堆叠页。按钮选中态由点击行为 + QButtonGroup 互斥机制维护。
    ui->mainPageStack->setCurrentWidget(page);
}

void MainWindow::setupHomePageBindings()
{
    // 首页项目变化 -> 写入成员变量并刷新底栏“当前项目”。
    if (m_homePage) {
        connect(m_homePage, &HomePage::currentProjectChanged, this, [this](const QString &projectName) {
            m_currentProjectName = projectName;
            setupStatusBarContent();
        });
    }
}
