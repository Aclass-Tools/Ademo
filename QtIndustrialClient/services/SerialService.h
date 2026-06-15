/**
 * @file SerialService.h
 * @brief 串口通信服务类，用于工业设备通信
 * @author Claude Code
 * @date 2026-04-10
 */

#pragma once

#include <functional>
#include <QSerialPort>
#include "services/ProtocolParser.h"

/**
 * @class SerialService
 * @brief 管理与嵌入式设备的串口通信(UART)
 *
 * 提供高级串口功能，包括：
 * - 可配置波特率的串口打开/关闭
 * - 使用ProtocolParser进行基于帧的通信
 * - 日志记录和帧接收回调
 * - 异步数据处理
 */
class SerialService : public QObject {
public:
    /**
     * @brief 构造一个新的串口服务对象
     * @param parent 用于内存管理的父QObject
     */
    explicit SerialService(QObject* parent = nullptr);

    /**
     * @brief 使用指定配置打开串口
     * @param portName 串口名称（如"COM1"、"/dev/ttyUSB0"）
     * @param baudRate 通信波特率
     * @param errorMessage 可选的错误信息输出参数
     * @return 如果串口成功打开返回true，否则返回false
     */
    bool openPort(const QString& portName, qint32 baudRate, QString* errorMessage = nullptr);

    /**
     * @brief 关闭当前打开的串口
     */
    void closePort();

    /**
     * @brief 检查串口是否当前已打开
     * @return 如果串口已打开返回true，否则返回false
     */
    bool isOpen() const;

    /**
     * @brief 通过串口发送数据帧
     * @param frame 包含帧数据的字节数组
     * @param errorMessage 可选的错误信息输出参数
     * @return 如果帧发送成功返回true，否则返回false
     */
    bool sendFrame(const QByteArray& frame, QString* errorMessage = nullptr);

    /**
     * @brief 设置日志消息的回调函数
     * @param callback 接受QString日志消息的std::function
     */
    void setLogCallback(std::function<void(const QString&)> callback);

    /**
     * @brief 设置接收帧的回调函数
     * @param callback 接受QByteArray帧的std::function
     */
    void setFrameReceivedCallback(std::function<void(const QByteArray&)> callback);

private:
    /**
     * @brief 处理从串口接收的数据
     * 当有数据可读时自动调用
     */
    void handleReadyRead();

    /**
     * @brief 使用注册的回调函数记录消息
     * @param message 要记录的消息
     */
    void log(const QString& message) const;

    QSerialPort m_serialPort;                              ///< Qt串口对象
    ProtocolParser m_parser;                               ///< 用于帧处理的协议解析器
    std::function<void(const QString&)> m_logCallback;    ///< 日志回调函数
    std::function<void(const QByteArray&)> m_frameReceivedCallback; ///< 帧接收回调函数
};
