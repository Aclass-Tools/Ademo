// 协议调试页面（ProtocolDebugPage）
// ---------------------------------
// 加载 .bin 协议描述 → 字段表展示 → 按字段值组包发送 / 把收到的字节解帧。
//
// 两层协议协作（核心）：
// - 字段表层（FrameCodec）：字段值 → payload 字节 / payload 字节 → 字段值。
// - 传输封装层（ProtocolParser）：payload → A0/A3 完整帧 / 完整帧 → payload。
// - 传输层（ConnectionManager）：经串口/网口收发完整帧。
//
// 重构后：不再持有 SerialPortManager，发送/接收全部走全局 ConnectionManager。

#pragma once

#include "placeholderpagebase.h"
#include "models/binprotocolloader.h"
#include "services/protocolparser.h"

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolDebugPage;
}
QT_END_NAMESPACE

class QLineEdit;
class ProtocolFieldTableModel;

namespace services { class ConnectionManager; }

class ProtocolDebugPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolDebugPage(QWidget *parent = nullptr);
    ~ProtocolDebugPage();
    void onPageActivated() override;
    // 由 MainWindow 注入共享连接管理器。
    void setConnectionManager(services::ConnectionManager *mgr);

private:
    void loadFromPath(const QString &path);
    void loadSample();
    void rebuildEncodeInputs();
    void onEncodeClicked();
    void onDecodeClicked();
    // 收到一帧（已封装层拆好的完整帧字节）→ 去帧头 → 解 payload → 刷新表格。
    void onFrameReceived(const QByteArray &frame);

private:
    Ui::ProtocolDebugPage *ui;
    ProtocolFieldTableModel *m_model = nullptr;
    BinProtocolLoader m_loader;
    // 字段名 → 输入控件（仅顶层；Struct 子字段用 "父名.子名" 作为 key）。
    QHash<QString, QLineEdit *> m_inputs;
    services::ConnectionManager *m_conn = nullptr;
    // 发送时用的源/目地址（默认：上位机 0x00 → 设备 0x01）。
    quint8 m_sourceId = 0x00;
    quint8 m_targetId = 0x01;
};
