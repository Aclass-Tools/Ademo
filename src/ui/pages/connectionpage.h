// 设备连接页（ConnectionPage）
// ------------------------------
// 全局连接的唯一配置入口：收集串口/网口参数，调用 ConnectionManager 建立连接。
// 所有页面共享同一个连接（由 MainWindow 持有的 ConnectionManager）。
//
// 职责：
// - 传输方式切换（串口 / 网口）。
// - 串口完整参数（端口/波特率/数据位/校验/停止位/流控）。
// - 网口参数（IP/端口）。
// - 连接/断开 + 状态显示。
//
// 不职责：不直接读写数据帧（由协议调试/指令操作页通过 ConnectionManager 收发）。

#pragma once

#include "placeholderpagebase.h"

#include <QString>

namespace services { class ConnectionManager; }

QT_BEGIN_NAMESPACE
namespace Ui {
class ConnectionPage;
}
QT_END_NAMESPACE

class ConnectionPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ConnectionPage(QWidget *parent = nullptr);
    ~ConnectionPage() override;

    // 由 MainWindow 注入共享连接管理器（与 setProjectContext 同模式）。
    void setConnectionManager(services::ConnectionManager *mgr);

protected:
    void onPageActivated() override;

private:
    void initCombos();
    void onModeChanged(int index);
    void onConnectClicked();
    void onConnectionChanged(bool connected);
    void refreshPorts();

    Ui::ConnectionPage *ui;
    services::ConnectionManager *m_conn = nullptr;
    bool m_disconnecting = false; // 区分"主动断开"与"底层掉线"，避免重复提示
};
