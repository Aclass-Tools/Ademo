#include "serialservice.h"

namespace services {

SerialService::SerialService(QObject *parent)
    : QObject(parent)
{
    // 裸字节：一方面直接发出（给终端），另一方面喂给 parser 拆帧。
    connect(&m_serial, &SerialPortManager::dataReceived, this, [this](const QByteArray &data) {
        emit rawBytesReceived(data);
        m_parser.appendData(data);
        const auto frames = m_parser.takeAvailableFrames();
        for (const QByteArray &f : frames) {
            emit frameReceived(f);
        }
    });
    connect(&m_serial, &SerialPortManager::portOpened, this, &SerialService::portOpened);
    connect(&m_serial, &SerialPortManager::portClosed, this, &SerialService::portClosed);
    connect(&m_serial, &SerialPortManager::errorOccurred, this, &SerialService::errorOccurred);
}

bool SerialService::openPort(const QString &portName, qint32 baudRate, int dataBits,
                             int parityIndex, int stopIndex, int flowIndex, QString *error)
{
    if (portName.isEmpty()) {
        if (error) *error = QStringLiteral("串口名为空");
        return false;
    }
    m_serial.setPortName(portName);
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(dataBits);
    m_serial.setParity(parityIndex);
    m_serial.setStopBits(stopIndex);
    m_serial.setFlowControl(flowIndex);
    if (!m_serial.openPort()) {
        if (error) *error = QStringLiteral("无法打开串口 %1").arg(portName);
        return false;
    }
    return true;
}

void SerialService::closePort()
{
    m_serial.closePort();
}

bool SerialService::isOpen() const
{
    return m_serial.isOpen();
}

bool SerialService::sendFrame(const QByteArray &frame, QString *error)
{
    if (!m_serial.isOpen()) {
        if (error) *error = QStringLiteral("串口未打开");
        return false;
    }
    const qint64 n = m_serial.sendData(frame);
    if (n < 0) {
        if (error) *error = QStringLiteral("串口发送失败");
        return false;
    }
    return true;
}

} // namespace services
