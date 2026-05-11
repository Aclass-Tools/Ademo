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
#include <QVector>
#include "ui/theme/thememanager.h"

class ApiClient;
class QButtonGroup;
class QLabel;
class QToolButton;
class QWidget;
class WebSocketClient;
class HomePage;
class ProtocolEditorPage;
class ProtocolDebugPage;
class ProtocolExportPage;
class DeviceUpgradePage;
class TerminalPage;
class LogPage;
class PluginPage;

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
    struct NavEntry
    {
        QToolButton *button = nullptr;
        QWidget *page = nullptr;
    };

    // 统一创建并挂载页面实例，同时组装导航映射、按钮初始化与导航点击绑定。
    void setupPages();
    // 统一处理底栏常驻显示（首次创建 + 基于成员变量刷新内容）。
    void setupStatusBarContent();
    // 切换并选中指定页面。
    void switchToPage(QWidget *page);
    // 绑定通信层信号：处理版本/错误/连接提示，
    void setupCommunicationBindings();
    // 用于显示临时通知，不承载长期状态。
    void showStatus(const QString &message);

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

    QVector<NavEntry> m_navEntries;
    QButtonGroup *m_navButtonGroup = nullptr;

    QLabel *m_connectionLabel = nullptr;
    QLabel *m_userLabel = nullptr;
    QLabel *m_configVersionLabel = nullptr;

    QString m_currentProjectName;
    QString m_currentUserName = QStringLiteral("engineer");
    QString m_currentConfigVersion;
};
