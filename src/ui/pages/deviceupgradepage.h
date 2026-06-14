// 设备升级页面（DeviceUpgradePage）
// ---------------------------------
// 选择固件 .bin → 经串口分块发送 → 显示进度与日志。
//
// 设计：
// - 独立的 SerialPortManager（与终端页互不影响）。
// - 用 QTimer 异步驱动逐块发送，避免阻塞事件循环。
// - 无设备时仍可“模拟”：分块流程跑通，但串口写返回 -1，日志体现。

#pragma once

#include "placeholderpagebase.h"
#include "network/serialportmanager.h"

#include <QByteArray>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class DeviceUpgradePage;
}
QT_END_NAMESPACE

class DeviceUpgradePage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit DeviceUpgradePage(QWidget *parent = nullptr);
    ~DeviceUpgradePage();
    void onPageActivated() override;

private:
    void refreshPorts();
    void populateBaudRates();
    void connectOrDisconnect();
    void browseFirmware();
    void startUpgrade();
    void cancelUpgrade();
    void sendNextChunk();
    void updateConnStatus(bool open);
    void log(const QString &msg);
    void setControlsRunning(bool running);

private:
    Ui::DeviceUpgradePage *ui;
    SerialPortManager m_serial;
    QTimer m_timer;

    QByteArray m_firmware;        // 完整固件字节
    int m_chunkSize = 256;
    int m_sentOffset = 0;         // 已发送字节偏移
    int m_totalSent = 0;
    bool m_running = false;
};
