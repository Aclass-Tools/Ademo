// 设备升级页面（DeviceUpgradePage）
// ---------------------------------
// 选择固件 .bin → 经全局 ConnectionManager 分块发送 → 显示进度与日志。
//
// 重构后：
// - 不再持有 SerialPortManager，连接由「设备连接」页统一管理。
// - 每块固件用 A0 写帧封装（mainCommand/subCommand 携带块号）发送。
// - 用 QTimer 异步驱动逐块发送，避免阻塞事件循环。
// - 未连接时仍可“模拟”：流程跑通，日志提示发送失败。

#pragma once

#include "placeholderpagebase.h"

#include <QByteArray>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class DeviceUpgradePage;
}
QT_END_NAMESPACE

namespace services { class ConnectionManager; }

class DeviceUpgradePage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit DeviceUpgradePage(QWidget *parent = nullptr);
    ~DeviceUpgradePage();
    void onPageActivated() override;
    void setConnectionManager(services::ConnectionManager *mgr);

private:
    void browseFirmware();
    void startUpgrade();
    void cancelUpgrade();
    void sendNextChunk();
    void log(const QString &msg);
    void setControlsRunning(bool running);
    void updateConnStatus(bool connected);

private:
    Ui::DeviceUpgradePage *ui;
    services::ConnectionManager *m_conn = nullptr;
    QTimer m_timer;

    QByteArray m_firmware;        // 完整固件字节
    int m_chunkSize = 256;
    int m_sentOffset = 0;         // 已发送字节偏移
    int m_totalSent = 0;
    int m_blockIndex = 0;         // 当前块号（从 1 起）
    bool m_running = false;
    quint8 m_sourceId = 0x00;
    quint8 m_targetId = 0x01;
};
