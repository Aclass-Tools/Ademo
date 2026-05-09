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
class QLabel;
class WebSocketClient;
class HomePage;
class ProtocolEditorPage;
class DeviceManagerPage;
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
    // 初始化状态栏指标控件。
    void setupNavigationTree();
    // 绑定顶层 UI 与基础设施信号/槽。
    void setupConnections();
    void showStatus(const QString &message);

private:
    ThemeKind m_themeKind = ThemeKind::IndustrialBlue;
    ThemePalette m_palette;

    Ui::MainWindow *ui = nullptr;

    ApiClient *m_apiClient = nullptr;
    WebSocketClient *m_wsClient = nullptr;

    HomePage *m_homePage = nullptr;
    ProtocolEditorPage *m_protocolEditorPage = nullptr;
    DeviceManagerPage *m_deviceManagerPage = nullptr;
    TerminalPage *m_terminalPage = nullptr;
    LogPage *m_logPage = nullptr;
    PluginPage *m_pluginPage = nullptr;

    QLabel *m_connectionLabel = nullptr;
    QLabel *m_userLabel = nullptr;
    QLabel *m_configVersionLabel = nullptr;
};
