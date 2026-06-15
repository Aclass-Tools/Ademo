// 主窗口（MainWindow）
// ----------------------
// Qt 桌面外壳的装配根节点。
//
// 职责：
// - 持有顶层导航按钮与堆叠页切换。
// - 持有跨页面基础设施客户端（HTTP + WebSocket）。
// - 将解耦页面组件注入容器布局。
//
// 非职责：
// - 各页面业务逻辑不应写在这里。
// - 传输解析与业务合并不应由外壳层处理。

#pragma once

#include <QMainWindow>
#include <memory>
#include "ui/theme/thememanager.h"

struct ProjectSummaryContext;

class ApiClient;
class QButtonGroup;
class QLabel;
class QToolButton;
class QWidget;
class WebSocketClient;
class PlaceholderPageBase;
class HomePage;
class ProtocolEditorPage;
class ProtocolDebugPage;
class ProtocolExportPage;
class DeviceUpgradePage;
class TerminalPage;
class LogPage;
class PluginPage;
class ConnectionPage;
class CommandOperationPage;

namespace services { class ConnectionManager; }

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    enum class PageKey {
        Home,
        ProtocolEditor,
        ProtocolDebug,
        ProtocolExport,
        DeviceUpgrade,
        Terminal,
        Log,
        Plugin,
        Connection,
        CommandOperation
    };
    // 初始化导航按钮行为。业务页改为“首次点击时创建（懒加载）”。
    void setupPages();
    // 根据页面键打开（创建/激活）对应标签页。
    void openPageTab(PageKey pageKey);
    // 标签关闭时释放页面资源，并清理页面指针。
    void closeTabByWidget(QWidget *tabWidget);
    // 首页导入项目成功后，关闭除首页外的所有标签页，回收页面资源。
    void closeAllNonHomeTabs();
    // 切换标签后触发页面激活回调。
    void onCurrentTabChanged(int index);
    // 首次访问时创建对应页面，并返回已创建实例。
    HomePage *ensureHomePage();
    ProtocolEditorPage *ensureProtocolEditorPage();
    ProtocolDebugPage *ensureProtocolDebugPage();
    ProtocolExportPage *ensureProtocolExportPage();
    DeviceUpgradePage *ensureDeviceUpgradePage();
    TerminalPage *ensureTerminalPage();
    LogPage *ensureLogPage();
    PluginPage *ensurePluginPage();
    ConnectionPage *ensureConnectionPage();
    CommandOperationPage *ensureCommandOperationPage();
    // 统一处理底栏常驻信息（首次创建标签 + 根据成员变量刷新文本）。
    void setupStatusBarContent();
    // 切换主区域当前页面（仅负责 stackedWidget 切换）。
    void switchToPage(QWidget *page);
    // 同步首页项目名到状态栏。
    void setupHomePageBindings();

private:
    ThemeKind m_themeKind = ThemeKind::IndustrialBlue;
    ThemePalette m_palette;

    Ui::MainWindow *ui = nullptr;

    ApiClient *m_apiClient = nullptr;
    WebSocketClient *m_wsClient = nullptr;

    HomePage *m_homePage = nullptr;
    ProtocolEditorPage *m_protocolEditorPage = nullptr;
    ProtocolDebugPage *m_protocolDebugPage = nullptr;
    ProtocolExportPage *m_protocolExportPage = nullptr;
    DeviceUpgradePage *m_deviceUpgradePage = nullptr;
    TerminalPage *m_terminalPage = nullptr;
    LogPage *m_logPage = nullptr;
    PluginPage *m_pluginPage = nullptr;
    ConnectionPage *m_connectionPage = nullptr;
    CommandOperationPage *m_commandOpPage = nullptr;

    // 全局共享连接管理器：所有页面通过它收发，不再各自持有串口。
    services::ConnectionManager *m_connManager = nullptr;

    QButtonGroup *m_navButtonGroup = nullptr;

    // 状态栏常驻五段信息（左3 + 右2）。
    QLabel *m_connectionLabel = nullptr;    // 当前项目描述
    QLabel *m_localDbLabel = nullptr;       // 本地数据库
    QLabel *m_remoteDbLabel = nullptr;      // 远程数据库
    QLabel *m_userLabel = nullptr;          // 当前用户（右侧）
    QLabel *m_configVersionLabel = nullptr; // 配置版本（右侧）

    // 跨页面共享项目摘要：HomePage 写入，MainWindow/其他页面只读。
    std::shared_ptr<ProjectSummaryContext> m_projectSummaryContext;
    bool m_homePageBindingsConnected = false;
    QString m_currentUserName = QStringLiteral("engineer");
};
