/**
 * @file SerialService.cpp
 * @brief 串口通信服务实现
 * @author Claude Code
 * @date 2026-04-10
 */

#include "services/SerialService.h"

#include <utility>

/**
 * @brief 构造串口服务对象
 * @param parent 父QObject
 */
SerialService::SerialService(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&m_serialPort, &QSerialPort::readyRead, this, [this]() {
        handleReadyRead();
    });
}

bool SerialService::openPort(const QString& portName, qint32 baudRate, QString* errorMessage)
{
    if (m_serialPort.isOpen()) {
        m_serialPort.close();
    }

    m_serialPort.setPortName(portName);
    m_serialPort.setBaudRate(baudRate);
    m_serialPort.setDataBits(QSerialPort::Data8);
    m_serialPort.setParity(QSerialPort::NoParity);
    m_serialPort.setStopBits(QSerialPort::OneStop);
    m_serialPort.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        if (errorMessage != nullptr) {
            *errorMessage = m_serialPort.errorString();
        }
        log(QStringLiteral("串口打开失败: %1").arg(m_serialPort.errorString()));
        return false;
    }

    log(QStringLiteral("串口已连接: %1 @ %2").arg(portName).arg(baudRate));
    return true;
}

void SerialService::closePort()
{
    if (!m_serialPort.isOpen()) {
        return;
    }

    const QString currentPort = m_serialPort.portName();
    m_serialPort.close();
    log(QStringLiteral("串口已关闭: %1").arg(currentPort));
}

bool SerialService::isOpen() const
{
    return m_serialPort.isOpen();
}

bool SerialService::sendFrame(const QByteArray& frame, QString* errorMessage)
{
    if (!m_serialPort.isOpen()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("串口未连接");
        }
        return false;
    }

    if (m_serialPort.write(frame) < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = m_serialPort.errorString();
        }
        log(QStringLiteral("发送失败: %1").arg(m_serialPort.errorString()));
        return false;
    }

    log(QStringLiteral("发送 %1 字节").arg(frame.size()));
    return true;
}

void SerialService::setLogCallback(std::function<void(const QString&)> callback)
{
    m_logCallback = std::move(callback);
}

void SerialService::setFrameReceivedCallback(std::function<void(const QByteArray&)> callback)
{
    m_frameReceivedCallback = std::move(callback);
}

void SerialService::handleReadyRead()
{
    m_parser.appendData(m_serialPort.readAll());
    const QVector<QByteArray> frames = m_parser.takeAvailableFrames();
    for (const QByteArray& frame : frames) {
        log(QStringLiteral("接收帧 %1 字节").arg(frame.size()));
        if (m_frameReceivedCallback) {
            m_frameReceivedCallback(frame);
        }
    }
}

void SerialService::log(const QString& message) const
{
    if (m_logCallback) {
        m_logCallback(message);
    }
}
