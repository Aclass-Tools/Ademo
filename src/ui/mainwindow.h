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
    // 创建各业务页并挂载到 mainwindow.ui 预留布局，同时初始化导航按钮行为。
    void setupPages();
    // 统一处理底栏常驻信息（首次创建标签 + 根据成员变量刷新文本）。
    void setupStatusBarContent();
    // 切换主区域当前页面（仅负责 stackedWidget 切换）。
    void switchToPage(QWidget *page);
    // 绑定通信层信号：处理版本回调、错误提示、连接提示与项目名同步。
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

    QButtonGroup *m_navButtonGroup = nullptr;

    // 状态栏常驻三段信息。
    QLabel *m_connectionLabel = nullptr;    // 当前项目
    QLabel *m_userLabel = nullptr;          // 当前用户
    QLabel *m_configVersionLabel = nullptr; // 配置版本

    // 状态栏文案源：所有常驻显示都从这些成员变量渲染。
    QString m_currentProjectName;
    QString m_currentUserName = QStringLiteral("engineer");
    QString m_currentConfigVersion;
};
