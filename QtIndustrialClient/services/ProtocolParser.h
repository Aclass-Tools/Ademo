/**
 * @file ProtocolParser.h
 * @brief 协议解析器，用于处理工业设备通信协议
 * @author Claude Code
 * @date 2026-04-10
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

#include "models/ParameterRecord.h"

/// 协议类型常量
constexpr quint8 PROTOCOL_A0 = 0xA0;
constexpr quint8 PROTOCOL_A3 = 0xA3;

/**
 * @brief 协议字符串转字节
 * @param str 协议字符串 ("A0" 或 "A3")
 * @return 协议字节值
 */
inline quint8 protocolStringToByte(const QString& str) {
    if (str.trimmed().compare("A3", Qt::CaseInsensitive) == 0)
        return PROTOCOL_A3;
    return PROTOCOL_A0;
}

/**
 * @brief 协议字节转字符串
 * @param type 协议字节值
 * @return 协议字符串 ("A0" 或 "A3")
 */
inline QString protocolByteToString(quint8 type) {
    if (type == PROTOCOL_A3)
        return QStringLiteral("A3");
    return QStringLiteral("A0");
}

/**
 * @struct ProtocolFrame
 * @brief 协议帧结构，表示解析后的完整通信帧
 *
 * 包含协议类型、源/目标地址、命令信息和负载数据，
 * 支持A0和A3协议类型的帧解析。
 */
struct ProtocolFrame
{
    quint8 protocolType = 0; ///< 协议类型 (0xA0 或 0xA3)
    quint8 sourceId = 0;     ///< 源地址
    quint8 targetId = 0;     ///< 目标地址
    quint8 frameType = 0;    ///< 帧类型
    quint8 mainCommand = 0;  ///< 主命令
    quint8 subCommand = 0;   ///< 子命令
    QByteArray payload;      ///< 有效负载数据
    bool isValid = false;    ///< 帧是否有效
};

/**
 * @class ProtocolParser
 * @brief 协议解析器类，用于帧的编码和解码
 *
 * 负责：
 * - 从字节流中提取完整帧
 * - 构建读/写命令帧
 * - 解析接收到的帧
 * - 计算Modbus CRC16校验和
 */
class ProtocolParser {
public:
    /**
     * @brief 向内部缓冲区追加数据
     * @param chunk 要追加的字节块
     * @return 剩余未解析的字节数据
     */
    QByteArray appendData(const QByteArray& chunk);

    /**
     * @brief 获取所有可用的完整帧
     * @return 包含完整帧字节数组的向量
     */
    QVector<QByteArray> takeAvailableFrames();

    /**
     * @brief 构建读命令帧
     * @param protocolType 协议类型 (0xA0 或 0xA3)
     * @param sourceId 源地址
     * @param targetId 目标地址
     * @param mainCommand 主命令
     * @param subCommand 子命令
     * @return 构建好的完整帧字节数组
     */
    static QByteArray buildReadFrame(quint8 protocolType, quint8 sourceId, quint8 targetId, quint8 mainCommand, quint8 subCommand);

    /**
     * @brief 构建写命令帧
     * @param protocolType 协议类型 (0xA0 或 0xA3)
     * @param sourceId 源地址
     * @param targetId 目标地址
     * @param mainCommand 主命令
     * @param subCommand 子命令
     * @param payload 要写入的有效负载数据
     * @return 构建好的完整帧字节数组
     */
    static QByteArray buildWriteFrame(quint8 protocolType, quint8 sourceId, quint8 targetId, quint8 mainCommand, quint8 subCommand, const QByteArray& payload);

    /**
     * @brief 编码参数记录的写操作负载
     * @param record 参数记录
     * @param valueOverride 可选的覆盖值
     * @return 编码后的字节数组
     */
    static QByteArray encodeWritePayload(const ParameterRecord& record, const QString& valueOverride = {});

    /**
     * @brief 解析字节数组为协议帧
     * @param frame 要解析的帧字节数组
     * @return 解析后的ProtocolFrame对象
     */
    static ProtocolFrame parseFrame(const QByteArray& frame);

private:
    /**
     * @brief 构建A0协议类型的帧
     * @param sourceId 源地址
     * @param targetId 目标地址
     * @param frameType 帧类型
     * @param mainCommand 主命令
     * @param subCommand 子命令
     * @param payload 有效负载数据
     * @return 构建好的A0协议帧
     */
    static QByteArray buildA0Frame(quint8 sourceId, quint8 targetId, quint8 frameType, quint8 mainCommand, quint8 subCommand, const QByteArray& payload);

    /**
     * @brief 构建A3协议类型的帧
     * @param sourceId 源地址
     * @param targetId 目标地址
     * @param frameType 帧类型
     * @param mainCommand 主命令
     * @param subCommand 子命令
     * @param payload 有效负载数据
     * @return 构建好的A3协议帧
     */
    static QByteArray buildA3Frame(quint8 sourceId, quint8 targetId, quint8 frameType, quint8 mainCommand, quint8 subCommand, const QByteArray& payload);

    /**
     * @brief 计算Modbus CRC16校验和
     * @param data 要计算校验和的字节数据
     * @return CRC16校验和值
     */
    static quint16 modbusCrc16(const QByteArray& data);

    QByteArray m_buffer;  ///< 内部缓冲区，用于累积和解析字节流
};
