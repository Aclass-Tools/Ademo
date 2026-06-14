// 终端页面（TerminalPage）
// -------------------------------
// 完整串口调试工具：参数配置、Hex/ASCII 收发、时间戳、收发计数、定时发送。
//
// 设计：
// - 持有独立的 SerialPortManager 实例（页面级，不跨页面共享）。
// - 进入页面时刷新可用串口列表（onPageActivated）。
// - 收发逻辑全部走 SerialPortManager 的信号/方法。

#pragma once

#include "placeholderpagebase.h"
#include "network/serialportmanager.h"

#include <QTimer>

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

private:
    void refreshPorts();
    void populateBaudRates();
    void applyConfigToSerial();
    void openOrClosePort();
    void onSendButtonClicked();
    void onDataReceived(const QByteArray &data);
    void appendRecv(const QByteArray &data);
    void updateConnStatus(bool open);
    void onTimerToggled(bool checked);

private:
    Ui::TerminalPage *ui;
    SerialPortManager m_serial;
    QTimer m_timer;
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
};
