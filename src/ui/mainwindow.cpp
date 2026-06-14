// MainWindow 实现
// ----------------
// 本文件负责装配顶层桌面外壳，并连接：
// - 导航按钮 -> 堆叠页切换
// - 基础设施客户端事件 -> 状态栏显示
//
// 页面内部细节已解耦到 src/ui/pages/*。

#include "mainwindow.h"

#include "models/projectsummarycontext.h"
#include "network/apiclient.h"
#include "network/websocketclient.h"
#include "ui/pages/homepage.h"
#include "ui/pages/placeholderpagebase.h"
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
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QWidget>

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
    // 创建跨页面共享的项目摘要上下文（同一份智能指针在各页面间传递）。
    m_projectSummaryContext = std::make_shared<ProjectSummaryContext>();

    // 先装配页面与导航，再做状态栏绑定，保证后续回调对象都已就绪。
    setupPages();

    // 主窗体基础样式由主题管理器统一生成。
    setStyleSheet(ThemeManager::mainWindowStyle(m_palette));

    // 初始化状态栏显示。
    setupStatusBarContent();
    setupHomePageBindings();
    ui->mainTabWidget->setTabsClosable(true);
    ui->mainTabWidget->setMovable(true);
    connect(ui->mainTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *tabWidget = ui->mainTabWidget->widget(index);
        if (!tabWidget) {
            return;
        }
        if (tabWidget == ui->pageHome) {
            return;
        }
        closeTabByWidget(tabWidget);
    });
    connect(ui->mainTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    ui->btnToolHome->setChecked(true);
    openPageTab(PageKey::Home);

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

    auto initNavButton = [this, navStyle](QToolButton *navButton) {
        if (!navButton) {
            return;
        }
        navButton->setCheckable(true);
        navButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        navButton->setStyleSheet(navStyle);
        m_navButtonGroup->addButton(navButton);
    };

    initNavButton(ui->btnToolHome);
    initNavButton(ui->btnProtocolEdit);
    initNavButton(ui->btnProtocolDebug);
    initNavButton(ui->btnDeviceConnect);
    initNavButton(ui->btnDeviceUpgrade);
    initNavButton(ui->btnTerminalDebug);
    initNavButton(ui->btnLogExport);
    initNavButton(ui->btnPluginScript);

    // 启动时先隐藏全部标签页，后续由用户点击按钮按需打开。
    while (ui->mainTabWidget->count() > 0) {
        ui->mainTabWidget->removeTab(0);
    }

    // 导航点击后首次创建页面，再打开/激活对应标签页。
    connect(ui->btnToolHome, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::Home);
    });
    connect(ui->btnProtocolEdit, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::ProtocolEditor);
    });
    connect(ui->btnProtocolDebug, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::ProtocolDebug);
    });
    connect(ui->btnDeviceConnect, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::ProtocolExport);
    });
    connect(ui->btnDeviceUpgrade, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::DeviceUpgrade);
    });
    connect(ui->btnTerminalDebug, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::Terminal);
    });
    connect(ui->btnLogExport, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::Log);
    });
    connect(ui->btnPluginScript, &QToolButton::clicked, this, [this]() {
        openPageTab(PageKey::Plugin);
    });
}

void MainWindow::setupStatusBarContent()
{
    // 延迟创建：首次调用时创建底栏标签并挂到状态栏。
    if (!m_connectionLabel) {
        m_connectionLabel = new QLabel(this);
        statusBar()->addWidget(m_connectionLabel);
    }
    if (!m_localDbLabel) {
        m_localDbLabel = new QLabel(this);
        statusBar()->addWidget(m_localDbLabel);
    }
    if (!m_remoteDbLabel) {
        m_remoteDbLabel = new QLabel(this);
        statusBar()->addWidget(m_remoteDbLabel);
    }
    if (!m_userLabel) {
        m_userLabel = new QLabel(this);
        statusBar()->addPermanentWidget(m_userLabel);
    }
    if (!m_configVersionLabel) {
        m_configVersionLabel = new QLabel(this);
        statusBar()->addPermanentWidget(m_configVersionLabel);
    }

    const QString projectName = m_projectSummaryContext ? m_projectSummaryContext->projectName : QString();
    const QString localDbAddress = m_projectSummaryContext ? m_projectSummaryContext->localDbAddress : QString();
    const QString remoteDbAddress = m_projectSummaryContext ? m_projectSummaryContext->remoteDbAddress : QString();
    const QString configVersion = m_projectSummaryContext ? m_projectSummaryContext->configVersion : QString();

    // 当前项目：空值回落到“未选择项目”。
    const QString shownProject = projectName.trimmed().isEmpty()
        ? QStringLiteral("未选择项目")
        : projectName.trimmed();
    m_connectionLabel->setText(QStringLiteral("当前项目描述: %1").arg(shownProject));

    // 本地数据库：空值回落到“--”。
    const QString shownLocalDb = localDbAddress.trimmed().isEmpty()
        ? QStringLiteral("--")
        : localDbAddress.trimmed();
    m_localDbLabel->setText(QStringLiteral("本地数据库: %1").arg(shownLocalDb));

    // 远程数据库：空值回落到“--”。
    const QString shownRemoteDb = remoteDbAddress.trimmed().isEmpty()
        ? QStringLiteral("--")
        : remoteDbAddress.trimmed();
    m_remoteDbLabel->setText(QStringLiteral("远程数据库: %1").arg(shownRemoteDb));

    // 当前用户：空值回落到“--”。
    const QString shownUser = m_currentUserName.trimmed().isEmpty()
        ? QStringLiteral("--")
        : m_currentUserName.trimmed();
    m_userLabel->setText(QStringLiteral("User: %1").arg(shownUser));

    // 配置版本：空值回落到“--”。
    const QString shownVersion = configVersion.trimmed().isEmpty()
        ? QStringLiteral("--")
        : configVersion.trimmed();
    m_configVersionLabel->setText(QStringLiteral("Config Version: %1").arg(shownVersion));
}

void MainWindow::switchToPage(QWidget *page)
{
    // 防御：空页面直接忽略，避免非法切换。
    if (!page) {
        return;
    }

    // 仅负责切换标签页。
    const int tabIndex = ui->mainTabWidget->indexOf(page);
    if (tabIndex < 0) {
        return;
    }
    ui->mainTabWidget->setCurrentIndex(tabIndex);
}

void MainWindow::onCurrentTabChanged(int index)
{
    if (index < 0) {
        return;
    }

    const QWidget *page = ui->mainTabWidget->widget(index);
    if (!page) {
        return;
    }

    // 标签切换后触发一次“页面已激活”回调，用于按需刷新。
    PlaceholderPageBase *activatedPage = nullptr;
    if (page == ui->pageHome) {
        activatedPage = m_homePage;
    } else if (page == ui->pageProtocolEditor) {
        activatedPage = m_protocolEditorPage;
    } else if (page == ui->pageProtocolDebug) {
        activatedPage = m_protocolDebugPage;
    } else if (page == ui->pageProtocolExport) {
        activatedPage = m_protocolExportPage;
    } else if (page == ui->pageDeviceUpgrade) {
        activatedPage = m_deviceUpgradePage;
    } else if (page == ui->pageTerminal) {
        activatedPage = m_terminalPage;
    } else if (page == ui->pageLog) {
        activatedPage = m_logPage;
    } else if (page == ui->pagePlugin) {
        activatedPage = m_pluginPage;
    }
    if (activatedPage) {
        activatedPage->onPageActivated();
    }
}

void MainWindow::openPageTab(PageKey pageKey)
{
    QWidget *tabWidget = nullptr;
    QString tabTitle;

    switch (pageKey) {
    case PageKey::Home:
        ensureHomePage();
        tabWidget = ui->pageHome;
        tabTitle = QStringLiteral("首页");
        break;
    case PageKey::ProtocolEditor:
        ensureProtocolEditorPage();
        tabWidget = ui->pageProtocolEditor;
        tabTitle = QStringLiteral("协议编辑");
        break;
    case PageKey::ProtocolDebug:
        ensureProtocolDebugPage();
        tabWidget = ui->pageProtocolDebug;
        tabTitle = QStringLiteral("协议调试");
        break;
    case PageKey::ProtocolExport:
        ensureProtocolExportPage();
        tabWidget = ui->pageProtocolExport;
        tabTitle = QStringLiteral("协议导出");
        break;
    case PageKey::DeviceUpgrade:
        ensureDeviceUpgradePage();
        tabWidget = ui->pageDeviceUpgrade;
        tabTitle = QStringLiteral("设备升级");
        break;
    case PageKey::Terminal:
        ensureTerminalPage();
        tabWidget = ui->pageTerminal;
        tabTitle = QStringLiteral("终端调试");
        break;
    case PageKey::Log:
        ensureLogPage();
        tabWidget = ui->pageLog;
        tabTitle = QStringLiteral("日志导出");
        break;
    case PageKey::Plugin:
        ensurePluginPage();
        tabWidget = ui->pagePlugin;
        tabTitle = QStringLiteral("外挂脚本");
        break;
    }

    if (!tabWidget) {
        return;
    }

    int tabIndex = ui->mainTabWidget->indexOf(tabWidget);
    if (tabIndex < 0) {
        tabIndex = ui->mainTabWidget->addTab(tabWidget, tabTitle);
        // 首页标签固定不可关闭：单独移除其右侧关闭按钮（×），
        // 其它标签保留 × 由 tabCloseRequested 处理。
        if (tabWidget == ui->pageHome) {
            ui->mainTabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, nullptr);
        }
    } else {
        ui->mainTabWidget->setTabText(tabIndex, tabTitle);
    }

    ui->mainTabWidget->setCurrentIndex(tabIndex);
}

void MainWindow::closeTabByWidget(QWidget *tabWidget)
{
    if (!tabWidget) {
        return;
    }

    if (tabWidget == ui->pageHome) {
        return;
    }

    const int tabIndex = ui->mainTabWidget->indexOf(tabWidget);
    if (tabIndex >= 0) {
        ui->mainTabWidget->removeTab(tabIndex);
    }

    if (tabWidget == ui->pageProtocolEditor) {
        delete m_protocolEditorPage;
        m_protocolEditorPage = nullptr;
    } else if (tabWidget == ui->pageProtocolDebug) {
        delete m_protocolDebugPage;
        m_protocolDebugPage = nullptr;
    } else if (tabWidget == ui->pageProtocolExport) {
        delete m_protocolExportPage;
        m_protocolExportPage = nullptr;
    } else if (tabWidget == ui->pageDeviceUpgrade) {
        delete m_deviceUpgradePage;
        m_deviceUpgradePage = nullptr;
    } else if (tabWidget == ui->pageTerminal) {
        delete m_terminalPage;
        m_terminalPage = nullptr;
    } else if (tabWidget == ui->pageLog) {
        delete m_logPage;
        m_logPage = nullptr;
    } else if (tabWidget == ui->pagePlugin) {
        delete m_pluginPage;
        m_pluginPage = nullptr;
    }
}

void MainWindow::setupHomePageBindings()
{
    // 首页导入项目后会先写共享上下文，再发更新信号；主窗口只负责刷新展示。
    if (m_homePage && !m_homePageBindingsConnected) {
        connect(m_homePage, &HomePage::projectSummaryChanged, this, [this]() {
            setupStatusBarContent();
            closeAllNonHomeTabs();
        });
        m_homePageBindingsConnected = true;
    }
}

void MainWindow::closeAllNonHomeTabs()
{
    // 复制当前标签页指针快照，避免关闭过程中 count/index 变化导致遍历失效。
    QVector<QWidget *> toClose;
    toClose.reserve(ui->mainTabWidget->count());
    for (int i = 0; i < ui->mainTabWidget->count(); ++i) {
        QWidget *tabWidget = ui->mainTabWidget->widget(i);
        if (!tabWidget || tabWidget == ui->pageHome) {
            continue;
        }
        toClose.push_back(tabWidget);
    }

    // 逐个关闭并释放对应页面实例。
    for (QWidget *tabWidget : toClose) {
        closeTabByWidget(tabWidget);
    }

    // 关闭完成后回到首页标签，确保视觉与交互一致。
    openPageTab(PageKey::Home);
}

HomePage *MainWindow::ensureHomePage()
{
    if (!m_homePage) {
        m_homePage = new HomePage(this);
        m_homePage->setProjectContext(m_projectSummaryContext);
        m_homePage->setWritableProjectContext(m_projectSummaryContext);
        if (ui->homeLayout) {
            ui->homeLayout->addWidget(m_homePage);
        }
        setupHomePageBindings();
    }
    return m_homePage;
}

ProtocolEditorPage *MainWindow::ensureProtocolEditorPage()
{
    if (!m_protocolEditorPage) {
        m_protocolEditorPage = new ProtocolEditorPage(this);
        m_protocolEditorPage->setProjectContext(m_projectSummaryContext);
        if (ui->protocolEditorLayout) {
            ui->protocolEditorLayout->addWidget(m_protocolEditorPage);
        }
    }
    return m_protocolEditorPage;
}

ProtocolDebugPage *MainWindow::ensureProtocolDebugPage()
{
    if (!m_protocolDebugPage) {
        m_protocolDebugPage = new ProtocolDebugPage(this);
        m_protocolDebugPage->setProjectContext(m_projectSummaryContext);
        if (ui->protocolDebugLayout) {
            ui->protocolDebugLayout->addWidget(m_protocolDebugPage);
        }
    }
    return m_protocolDebugPage;
}

ProtocolExportPage *MainWindow::ensureProtocolExportPage()
{
    if (!m_protocolExportPage) {
        m_protocolExportPage = new ProtocolExportPage(this);
        m_protocolExportPage->setProjectContext(m_projectSummaryContext);
        if (ui->protocolExportLayout) {
            ui->protocolExportLayout->addWidget(m_protocolExportPage);
        }
    }
    return m_protocolExportPage;
}

DeviceUpgradePage *MainWindow::ensureDeviceUpgradePage()
{
    if (!m_deviceUpgradePage) {
        m_deviceUpgradePage = new DeviceUpgradePage(this);
        m_deviceUpgradePage->setProjectContext(m_projectSummaryContext);
        if (ui->deviceUpgradeLayout) {
            ui->deviceUpgradeLayout->addWidget(m_deviceUpgradePage);
        }
    }
    return m_deviceUpgradePage;
}

TerminalPage *MainWindow::ensureTerminalPage()
{
    if (!m_terminalPage) {
        m_terminalPage = new TerminalPage(this);
        m_terminalPage->setProjectContext(m_projectSummaryContext);
        if (ui->terminalLayout) {
            ui->terminalLayout->addWidget(m_terminalPage);
        }
    }
    return m_terminalPage;
}

LogPage *MainWindow::ensureLogPage()
{
    if (!m_logPage) {
        m_logPage = new LogPage(this);
        m_logPage->setProjectContext(m_projectSummaryContext);
        if (ui->logLayout) {
            ui->logLayout->addWidget(m_logPage);
        }
    }
    return m_logPage;
}

PluginPage *MainWindow::ensurePluginPage()
{
    if (!m_pluginPage) {
        m_pluginPage = new PluginPage(this);
        m_pluginPage->setProjectContext(m_projectSummaryContext);
        if (ui->pluginLayout) {
            ui->pluginLayout->addWidget(m_pluginPage);
        }
    }
    return m_pluginPage;
}
