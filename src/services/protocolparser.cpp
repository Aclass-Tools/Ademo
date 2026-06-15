#include "protocolparser.h"

#include <QtEndian>

namespace services {
namespace {

constexpr char kSyncByteA0 = char(0xA0);
constexpr char kSyncByteA3 = char(0xA3);
// A3 帧结束符：回车 + 换行。
constexpr char kA3FrameEndFirst = char(0x0D);
constexpr char kA3FrameEndSecond = char(0x0A);

// 把数值按小端写成定长字节。
template <typename T>
QByteArray toLittleEndianBytes(T value)
{
    QByteArray bytes(int(sizeof(T)), Qt::Uninitialized);
    qToLittleEndian(value, reinterpret_cast<uchar *>(bytes.data()));
    return bytes;
}

} // namespace

void ProtocolParser::appendData(const QByteArray &chunk)
{
    m_buffer.append(chunk);
}

QVector<QByteArray> ProtocolParser::takeAvailableFrames()
{
    QVector<QByteArray> frames;

    while (m_buffer.size() >= 4) {
        // 找第一个同步字节。
        int syncIndex = -1;
        for (int i = 0; i < m_buffer.size(); ++i) {
            const char v = m_buffer.at(i);
            if (v == kSyncByteA0 || v == kSyncByteA3) {
                syncIndex = i;
                break;
            }
        }
        if (syncIndex < 0) {
            m_buffer.clear();
            break;
        }
        if (syncIndex > 0) {
            m_buffer.remove(0, syncIndex);
        }

        const char syncByte = m_buffer.at(0);

        if (syncByte == kSyncByteA0) {
            // A0：len 在偏移 1（1 字节），帧总长 = 2 + len。
            if (m_buffer.size() < 9) {
                break;
            }
            const quint8 payloadLen = quint8(m_buffer.at(1));
            const int frameLen = 2 + int(payloadLen);
            if (m_buffer.size() < frameLen) {
                break;
            }
            const QByteArray frame = m_buffer.left(frameLen);
            // CRC 校验：帧去掉末尾 2 字节 CRC。
            const QByteArray content = frame.left(frameLen - 2);
            const quint16 expectedCrc =
                quint8(frame.at(frameLen - 2)) | (quint8(frame.at(frameLen - 1)) << 8);
            if (modbusCrc16(content) == expectedCrc) {
                frames.push_back(frame);
                m_buffer.remove(0, frameLen);
            } else {
                // CRC 错，跳过同步字节继续找。
                m_buffer.remove(0, 1);
            }
            continue;
        }

        if (syncByte == kSyncByteA3) {
            // A3：len 在偏移 1..2（大端 2 字节），帧总长 = 3 + len。
            if (m_buffer.size() < 11) {
                break;
            }
            const quint16 payloadLen =
                (quint8(m_buffer.at(1)) << 8) | quint8(m_buffer.at(2));
            const int frameLen = 3 + int(payloadLen);
            if (m_buffer.size() < frameLen) {
                break;
            }
            const QByteArray frame = m_buffer.left(frameLen);
            if (frame.at(frameLen - 2) == kA3FrameEndFirst &&
                frame.at(frameLen - 1) == kA3FrameEndSecond) {
                frames.push_back(frame);
                m_buffer.remove(0, frameLen);
            } else {
                m_buffer.remove(0, 1);
            }
            continue;
        }
    }

    return frames;
}

QByteArray ProtocolParser::buildReadFrame(quint8 protocolType, quint8 sourceId, quint8 targetId,
                                          quint8 mainCommand, quint8 subCommand)
{
    if (protocolType == kProtocolA3) {
        return buildA3Frame(sourceId, targetId, kFrameTypeRead, mainCommand, subCommand, {});
    }
    return buildA0Frame(sourceId, targetId, kFrameTypeRead, mainCommand, subCommand, {});
}

QByteArray ProtocolParser::buildWriteFrame(quint8 protocolType, quint8 sourceId, quint8 targetId,
                                           quint8 mainCommand, quint8 subCommand, const QByteArray &payload)
{
    if (protocolType == kProtocolA3) {
        return buildA3Frame(sourceId, targetId, kFrameTypeWrite, mainCommand, subCommand, payload);
    }
    return buildA0Frame(sourceId, targetId, kFrameTypeWrite, mainCommand, subCommand, payload);
}

ProtocolFrame ProtocolParser::parseFrame(const QByteArray &frame)
{
    ProtocolFrame parsed;
    if (frame.isEmpty()) {
        return parsed;
    }

    const char syncByte = frame.at(0);

    if (syncByte == kSyncByteA0) {
        if (frame.size() < 9) {
            return parsed;
        }
        parsed.protocolType = kProtocolA0;
        parsed.sourceId = quint8(frame.at(2));
        parsed.targetId = quint8(frame.at(3));
        parsed.frameType = quint8(frame.at(4));
        parsed.mainCommand = quint8(frame.at(5));
        parsed.subCommand = quint8(frame.at(6));
        // payload = src..subCmd(5 字节) + payload，去掉头 7 与尾 2(CRC)。
        parsed.payload = frame.mid(7, frame.size() - 9);
        parsed.isValid = true;
        return parsed;
    }

    if (syncByte == kSyncByteA3) {
        if (frame.size() < 11) {
            return parsed;
        }
        parsed.protocolType = kProtocolA3;
        parsed.sourceId = quint8(frame.at(3));
        parsed.targetId = quint8(frame.at(4));
        parsed.frameType = quint8(frame.at(5));
        parsed.mainCommand = quint8(frame.at(6));
        parsed.subCommand = quint8(frame.at(7));
        // payload：跳过头 3 + 固定 6 + 1(保留)，去掉尾 2(0x0D0x0A)。
        parsed.payload = frame.mid(9, frame.size() - 11);
        parsed.isValid = true;
    }

    return parsed;
}

QByteArray ProtocolParser::buildA0Frame(quint8 sourceId, quint8 targetId, quint8 frameType,
                                        quint8 mainCommand, quint8 subCommand, const QByteArray &payload)
{
    QByteArray frame;
    frame.append(kSyncByteA0);
    frame.append('\0'); // len 占位
    frame.append(char(sourceId));
    frame.append(char(targetId));
    frame.append(char(frameType));
    frame.append(char(mainCommand));
    frame.append(char(subCommand));
    frame.append(payload);

    // len = 从 src 起到 crc 前的总字节数 + 2（约定与参考项目一致）。
    const quint8 length = quint8((frame.size() - 2) + 2);
    frame[1] = char(length);

    const quint16 crc = modbusCrc16(frame);
    frame.append(char(crc & 0xFF));
    frame.append(char((crc >> 8) & 0xFF));
    return frame;
}

QByteArray ProtocolParser::buildA3Frame(quint8 sourceId, quint8 targetId, quint8 frameType,
                                        quint8 mainCommand, quint8 subCommand, const QByteArray &payload)
{
    QByteArray frame;
    frame.append(kSyncByteA3);
    frame.append('\0'); // lenHi 占位
    frame.append('\0'); // lenLo 占位
    frame.append(char(sourceId));
    frame.append(char(targetId));
    frame.append(char(frameType));
    frame.append(char(mainCommand));
    frame.append(char(subCommand));
    frame.append('\0'); // 保留
    frame.append(payload);
    frame.append(kA3FrameEndFirst);
    frame.append(kA3FrameEndSecond);

    const quint16 length = quint16(frame.size() - 3);
    frame[1] = char((length >> 8) & 0xFF);
    frame[2] = char(length & 0xFF);
    return frame;
}

quint16 ProtocolParser::modbusCrc16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (const char byte : data) {
        crc ^= quint8(byte);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001) {
                crc = quint16((crc >> 1) ^ 0xA001);
            } else {
                crc = quint16(crc >> 1);
            }
        }
    }
    return crc;
}

} // namespace services
