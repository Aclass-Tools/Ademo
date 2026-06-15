/**
 * @file ProtocolParser.cpp
 * @brief 协议解析器实现
 * @author Claude Code
 * @date 2026-04-10
 */

#include "services/ProtocolParser.h"

#include <QtEndian>

namespace {
constexpr char kSyncByteA0 = static_cast<char>(0xA0);           ///< A0协议同步字节
constexpr char kSyncByteA3 = static_cast<char>(0xA3);           ///< A3协议同步字节
constexpr quint8 kReadType = 0x51;                                ///< 读取命令类型
constexpr quint8 kWriteType = 0x53;                               ///< 写入命令类型
constexpr char kA3FrameEndFirst = static_cast<char>(0x0D);      ///< A3帧结束符1（回车）
constexpr char kA3FrameEndSecond = static_cast<char>(0x0A);     ///< A3帧结束符2（换行）

template <typename T>
QByteArray numberToLittleEndianBytes(T value)
{
    QByteArray bytes(sizeof(T), Qt::Uninitialized);
    qToLittleEndian(value, reinterpret_cast<uchar*>(bytes.data()));
    return bytes;
}
}

QByteArray ProtocolParser::appendData(const QByteArray& chunk)
{
    m_buffer.append(chunk);
    return m_buffer;
}

QVector<QByteArray> ProtocolParser::takeAvailableFrames()
{
    QVector<QByteArray> frames;

    while (m_buffer.size() >= 4) {
        int syncIndex = -1;
        for (int i = 0; i < m_buffer.size(); ++i) {
            const char value = m_buffer.at(i);
            if (value == kSyncByteA0 || value == kSyncByteA3) {
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
            if (m_buffer.size() < 9) {
                break;
            }

            const quint8 payloadLength = static_cast<quint8>(m_buffer.at(1));
            const int frameLength = 2 + payloadLength;
            if (m_buffer.size() < frameLength) {
                break;
            }

            const QByteArray frame = m_buffer.left(frameLength);
            const QByteArray content = frame.left(frameLength - 2);
            const quint16 expectedCrc =
                static_cast<quint8>(frame.at(frameLength - 2)) |
                (static_cast<quint8>(frame.at(frameLength - 1)) << 8);

            if (modbusCrc16(content) == expectedCrc) {
                frames.push_back(frame);
                m_buffer.remove(0, frameLength);
            } else {
                m_buffer.remove(0, 1);
            }
            continue;
        }

        if (syncByte == kSyncByteA3) {
            if (m_buffer.size() < 11) {
                break;
            }

            const quint16 payloadLength =
                (static_cast<quint8>(m_buffer.at(1)) << 8) |
                static_cast<quint8>(m_buffer.at(2));
            const int frameLength = 3 + payloadLength;
            if (m_buffer.size() < frameLength) {
                break;
            }

            const QByteArray frame = m_buffer.left(frameLength);
            if (frame.at(frameLength - 2) == kA3FrameEndFirst &&
                frame.at(frameLength - 1) == kA3FrameEndSecond) {
                frames.push_back(frame);
                m_buffer.remove(0, frameLength);
            } else {
                m_buffer.remove(0, 1);
            }
            continue;
        }
    }

    return frames;
}

QByteArray ProtocolParser::buildReadFrame(quint8 protocolType, quint8 sourceId, quint8 targetId, quint8 mainCommand, quint8 subCommand)
{
    if (protocolType == PROTOCOL_A3) {
        return buildA3Frame(sourceId, targetId, kReadType, mainCommand, subCommand, {});
    }
    return buildA0Frame(sourceId, targetId, kReadType, mainCommand, subCommand, {});
}

QByteArray ProtocolParser::buildWriteFrame(quint8 protocolType, quint8 sourceId, quint8 targetId, quint8 mainCommand, quint8 subCommand, const QByteArray& payload)
{
    if (protocolType == PROTOCOL_A3) {
        return buildA3Frame(sourceId, targetId, kWriteType, mainCommand, subCommand, payload);
    }
    return buildA0Frame(sourceId, targetId, kWriteType, mainCommand, subCommand, payload);
}

QByteArray ProtocolParser::encodeWritePayload(const ParameterRecord& record, const QString& valueOverride)
{
    const QString rawValue = valueOverride.isEmpty() ? record.defaultValue.trimmed() : valueOverride.trimmed();
    const QString type = record.dataType.trimmed().toLower();

    if (type == QStringLiteral("uint8_t")) {
        return QByteArray(1, static_cast<char>(rawValue.isEmpty() ? 0 : rawValue.toUInt()));
    }
    if (type == QStringLiteral("int8_t")) {
        return QByteArray(1, static_cast<char>(rawValue.isEmpty() ? 0 : rawValue.toInt()));
    }
    if (type == QStringLiteral("uint16_t")) {
        return numberToLittleEndianBytes<quint16>(rawValue.isEmpty() ? 0 : static_cast<quint16>(rawValue.toUShort()));
    }
    if (type == QStringLiteral("int16_t")) {
        return numberToLittleEndianBytes<qint16>(rawValue.isEmpty() ? 0 : static_cast<qint16>(rawValue.toShort()));
    }
    if (type == QStringLiteral("uint32_t")) {
        return numberToLittleEndianBytes<quint32>(rawValue.isEmpty() ? 0 : rawValue.toUInt());
    }
    if (type == QStringLiteral("int32_t")) {
        return numberToLittleEndianBytes<qint32>(rawValue.isEmpty() ? 0 : rawValue.toInt());
    }
    if (type == QStringLiteral("float")) {
        return numberToLittleEndianBytes<float>(rawValue.isEmpty() ? 0.0f : rawValue.toFloat());
    }
    if (type == QStringLiteral("double")) {
        return numberToLittleEndianBytes<double>(rawValue.isEmpty() ? 0.0 : rawValue.toDouble());
    }
    if (type == QStringLiteral("int64_t")) {
        return numberToLittleEndianBytes<qint64>(rawValue.isEmpty() ? 0 : rawValue.toLongLong());
    }
    if (type == QStringLiteral("uint64_t")) {
        return numberToLittleEndianBytes<quint64>(rawValue.isEmpty() ? 0 : rawValue.toULongLong());
    }
    if (type == QStringLiteral("string")) {
        QByteArray bytes = rawValue.toLatin1();
        if (record.dataLength > 0 && bytes.size() > record.dataLength) {
            bytes.truncate(record.dataLength);
        }
        return bytes;
    }

    QByteArray fallback;
    if (!rawValue.isEmpty()) {
        fallback = rawValue.toLatin1();
    } else if (record.dataLength > 0) {
        fallback.fill('\0', record.dataLength);
    }
    return fallback;
}

ProtocolFrame ProtocolParser::parseFrame(const QByteArray& frame)
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

        parsed.protocolType = PROTOCOL_A0;
        parsed.sourceId = static_cast<quint8>(frame.at(2));
        parsed.targetId = static_cast<quint8>(frame.at(3));
        parsed.frameType = static_cast<quint8>(frame.at(4));
        parsed.mainCommand = static_cast<quint8>(frame.at(5));
        parsed.subCommand = static_cast<quint8>(frame.at(6));
        parsed.payload = frame.mid(7, frame.size() - 9);
        parsed.isValid = true;
        return parsed;
    }

    if (syncByte == kSyncByteA3) {
        if (frame.size() < 11) {
            return parsed;
        }

        parsed.protocolType = PROTOCOL_A3;
        parsed.sourceId = static_cast<quint8>(frame.at(3));
        parsed.targetId = static_cast<quint8>(frame.at(4));
        parsed.frameType = static_cast<quint8>(frame.at(5));
        parsed.mainCommand = static_cast<quint8>(frame.at(6));
        parsed.subCommand = static_cast<quint8>(frame.at(7));
        parsed.payload = frame.mid(9, frame.size() - 11);
        parsed.isValid = true;
    }

    return parsed;
}

QByteArray ProtocolParser::buildA0Frame(quint8 sourceId, quint8 targetId, quint8 frameType, quint8 mainCommand, quint8 subCommand, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(kSyncByteA0);
    frame.append('\0');
    frame.append(static_cast<char>(sourceId));
    frame.append(static_cast<char>(targetId));
    frame.append(static_cast<char>(frameType));
    frame.append(static_cast<char>(mainCommand));
    frame.append(static_cast<char>(subCommand));
    frame.append(payload);

    const quint8 length = static_cast<quint8>((frame.size() - 2) + 2);
    frame[1] = static_cast<char>(length);

    const quint16 crc = modbusCrc16(frame);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    return frame;
}

QByteArray ProtocolParser::buildA3Frame(quint8 sourceId, quint8 targetId, quint8 frameType, quint8 mainCommand, quint8 subCommand, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(kSyncByteA3);
    frame.append('\0');
    frame.append('\0');
    frame.append(static_cast<char>(sourceId));
    frame.append(static_cast<char>(targetId));
    frame.append(static_cast<char>(frameType));
    frame.append(static_cast<char>(mainCommand));
    frame.append(static_cast<char>(subCommand));
    frame.append('\0');
    frame.append(payload);
    frame.append(kA3FrameEndFirst);
    frame.append(kA3FrameEndSecond);

    const quint16 length = static_cast<quint16>(frame.size() - 3);
    frame[1] = static_cast<char>((length >> 8) & 0xFF);
    frame[2] = static_cast<char>(length & 0xFF);
    return frame;
}

quint16 ProtocolParser::modbusCrc16(const QByteArray& data)
{
    quint16 crc = 0xFFFF;
    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001) {
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            } else {
                crc = static_cast<quint16>(crc >> 1);
            }
        }
    }
    return crc;
}
