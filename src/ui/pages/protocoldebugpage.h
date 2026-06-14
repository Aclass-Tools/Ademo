// 协议调试页面（ProtocolDebugPage）
// ---------------------------------
// 加载 .bin 协议描述 → 字段表展示 → 按字段值组包发送 / 把收到的字节解帧。
//
// 设计：
// - 组包输入控件按字段动态生成（一字段一行 QLineEdit）。
// - 解帧：手动粘贴 hex 解析；后续若接入串口，可由 dataReceived 自动调用 decode。

#pragma once

#include "placeholderpagebase.h"
#include "models/binprotocolloader.h"
#include "network/serialportmanager.h"

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolDebugPage;
}
QT_END_NAMESPACE

class QLineEdit;
class ProtocolFieldTableModel;

class ProtocolDebugPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolDebugPage(QWidget *parent = nullptr);
    ~ProtocolDebugPage();
    void onPageActivated() override;

private:
    void loadFromPath(const QString &path);
    void loadSample();
    void rebuildEncodeInputs();
    void onEncodeClicked();
    void onDecodeClicked();
    void refreshPorts();

private:
    Ui::ProtocolDebugPage *ui;
    ProtocolFieldTableModel *m_model = nullptr;
    BinProtocolLoader m_loader;
    // 字段名 → 输入控件（仅顶层；Struct 子字段用 "父名.子名" 作为 key）。
    QHash<QString, QLineEdit *> m_inputs;
    // 页面内可选的串口（用于"经串口发送"）。
    SerialPortManager m_serial;
};
