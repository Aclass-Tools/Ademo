// 终端页面（TerminalPage）
// -------------------------------
// 原始数据收发视图：订阅全局 ConnectionManager 的裸字节流，发送也走全局连接。
//
// 设计（重构后）：
// - 不再持有独立 SerialPortManager，改为共享 MainWindow 的 ConnectionManager。
// - 串口配置/连接由「设备连接」页统一管理；本页只负责收发显示。
// - 接收：订阅 rawBytesReceived（不经协议拆帧的裸字节）。
// - 发送：通过 sendRaw 发送任意 hex/ascii 字节。
// - 保留 Hex/ASCII 显示、时间戳、自动换行、收发计数、定时发送。

#pragma once

#include "placeholderpagebase.h"

#include <QTimer>

namespace services { class ConnectionManager; }

QT_BEGIN_NAMESPACE
namespace Ui {
class TerminalPage;
}
QT_END_NAMESPACE

class TerminalPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit TerminalPage(QWidget *parent = nullptr);
    ~TerminalPage();
    void onPageActivated() override;
    // 由 MainWindow 注入共享连接管理器。
    void setConnectionManager(services::ConnectionManager *mgr);

private:
    void onSendButtonClicked();
    void onDataReceived(const QByteArray &data);
    void appendRecv(const QByteArray &data);
    void updateConnStatus(bool connected);
    void onTimerToggled(bool checked);

private:
    Ui::TerminalPage *ui;
    services::ConnectionManager *m_conn = nullptr;
    QTimer m_timer;
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
};
