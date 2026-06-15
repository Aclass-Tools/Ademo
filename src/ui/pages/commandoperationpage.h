// 指令操作页（CommandOperationPage）
// ------------------------------------
// 按字段对设备进行读/写操作（借鉴 QtIndustrialClient 的 CommandOperationPage）。
//
// 数据源：当前加载的 .bin 协议字段表（与协议调试页共用同一套字段定义）。
// 操作：
// - 单行"读"：对该字段发 A0 读帧。
// - 单行"写"：用输入框的值编码后发 A0 写帧。
// - "总召"：对所有字段依次发读帧。
// - "总写"：对所有字段依次发写帧。
//
// 收到的回包经 ConnectionManager::frameReceived → ProtocolParser 解析 → 回填表格。
// 不持有串口，全部走全局 ConnectionManager。

#pragma once

#include "placeholderpagebase.h"
#include "models/binprotocolloader.h"
#include "services/protocolparser.h"

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class CommandOperationPage;
}
QT_END_NAMESPACE

class QTableWidget;
class QTableWidgetItem;
class QLineEdit;

namespace services { class ConnectionManager; }

class CommandOperationPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit CommandOperationPage(QWidget *parent = nullptr);
    ~CommandOperationPage() override;
    void onPageActivated() override;
    void setConnectionManager(services::ConnectionManager *mgr);

private:
    void loadFromPath(const QString &path);
    void loadSample();
    void rebuildTable();
    // 对指定行发读/写帧。row=-1 表示全部行（总召/总写）。
    void sendRead(int row);
    void sendWrite(int row);
    void onReadAllClicked();
    void onWriteAllClicked();
    // 收到完整帧 → 解析 → 回填匹配行的值。
    void onFrameReceived(const QByteArray &frame);

    // 把字段值按字段类型编码为 payload（与 FrameCodec 一致的最小实现）。
    QByteArray encodeFieldValue(const FieldDef &f, const QString &text) const;
    QString decodeFieldValue(const FieldDef &f, const QByteArray &payload, int offset) const;

private:
    Ui::CommandOperationPage *ui;
    services::ConnectionManager *m_conn = nullptr;
    BinProtocolLoader m_loader;
    // 每行的字段定义快照（与表格行号对应）。
    QVector<FieldDef> m_rowFields;
    // 行号 → 值输入控件。
    QHash<int, QLineEdit *> m_valueEdits;
    // 行号 → 读/写按钮（便于总操作时遍历）。
    quint8 m_sourceId = 0x00;
    quint8 m_targetId = 0x01;
};
