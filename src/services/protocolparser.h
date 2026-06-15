// 协议解析器（ProtocolParser）
// --------------------------------
// 传输封装层：A0 / A3 帧的组帧、拆帧、CRC 校验。
//
// 与字段表层（FrameCodec）正交：
// - FrameCodec 负责 payload 内部字段如何排布。
// - ProtocolParser 负责 payload 外面包什么帧头 / 地址 / CRC。
//
// A0 帧（变长，CRC16 收尾，小端长度）：
//   [0xA0][len(1)][src(1)][dst(1)][frameType(1)][mainCmd(1)][subCmd(1)][payload...][crcLo][crcHi]
//   len = 从 src 起到 crc 前的总字节数 + 2（含自身后续约定，与参考项目一致）
//
// A3 帧（变长，0x0D 0x0A 收尾，大端长度）：
//   [0xA3][lenHi][lenLo][src][dst][frameType][mainCmd][subCmd][0x00][payload...][0x0D][0x0A]
//
// 所有方法无 Qt 依赖（仅用 QByteArray），便于单测。

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

namespace services {

// 协议类型常量（传输封装层）。
constexpr quint8 kProtocolA0 = 0xA0;
constexpr quint8 kProtocolA3 = 0xA3;

// 帧类型：读取 / 写入（约定值与参考项目一致）。
constexpr quint8 kFrameTypeRead = 0x51;
constexpr quint8 kFrameTypeWrite = 0x53;
// 设备主动上报的数据帧类型（收到后用于触发解 payload）。
constexpr quint8 kFrameTypeData = 0x54;

// 协议字符串 <-> 字节。
inline quint8 protocolStringToByte(const QString &s)
{
    return (s.trimmed().compare(QStringLiteral("A3"), Qt::CaseInsensitive) == 0)
        ? kProtocolA3 : kProtocolA0;
}
inline QString protocolByteToString(quint8 type)
{
    return (type == kProtocolA3) ? QStringLiteral("A3") : QStringLiteral("A0");
}

// 解析后的完整帧。
struct ProtocolFrame
{
    quint8 protocolType = 0;  // kProtocolA0 / kProtocolA3
    quint8 sourceId = 0;      // 源地址
    quint8 targetId = 0;      // 目标地址
    quint8 frameType = 0;     // 帧类型（读/写/数据）
    quint8 mainCommand = 0;   // 主命令
    quint8 subCommand = 0;    // 子命令
    QByteArray payload;       // 有效负载（字段表层 payload）
    bool isValid = false;     // 是否解析成功
};

class ProtocolParser
{
public:
    // 把收到的字节块追加到内部缓冲区。
    void appendData(const QByteArray &chunk);

    // 取出缓冲区里所有完整的帧（处理粘包/半包）。取走后从缓冲区移除。
    QVector<QByteArray> takeAvailableFrames();

    // ---- 组帧（静态，无状态） ----

    // 构建读取命令帧。
    static QByteArray buildReadFrame(quint8 protocolType, quint8 sourceId, quint8 targetId,
                                     quint8 mainCommand, quint8 subCommand);
    // 构建写入命令帧（payload 为字段表层 encode 出来的字节）。
    static QByteArray buildWriteFrame(quint8 protocolType, quint8 sourceId, quint8 targetId,
                                      quint8 mainCommand, quint8 subCommand, const QByteArray &payload);

    // 解析一帧字节为结构化 ProtocolFrame。
    static ProtocolFrame parseFrame(const QByteArray &frame);

    // Modbus CRC16。
    static quint16 modbusCrc16(const QByteArray &data);

private:
    static QByteArray buildA0Frame(quint8 sourceId, quint8 targetId, quint8 frameType,
                                   quint8 mainCommand, quint8 subCommand, const QByteArray &payload);
    static QByteArray buildA3Frame(quint8 sourceId, quint8 targetId, quint8 frameType,
                                   quint8 mainCommand, quint8 subCommand, const QByteArray &payload);

    QByteArray m_buffer;  // 接收缓冲区
};

} // namespace services
