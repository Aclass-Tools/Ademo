#include "serialportmanager.h"

#include <QSerialPort>
#include <QSerialPortInfo>

SerialPortManager::SerialPortManager(QObject *parent)
    : QObject(parent)
    , m_port(new QSerialPort(this))
{
    // 把底层信号重新映射为稳定的事件面，避免上层依赖 QSerialPort。
    connect(m_port, &QSerialPort::readyRead, this, [this]() {
        const QByteArray data = m_port->readAll();
        if (!data.isEmpty()) {
            emit dataReceived(data);
        }
    });
    // QSerialPort 没有独立的 opened/closed 信号，这里靠 openPort/closePort
    // 主动 emit，保证上层 UI 能稳定刷新连接状态。
    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        // ResourceError 在正常关闭时也会触发，过滤掉噪音。
        if (error == QSerialPort::NoError || error == QSerialPort::ResourceError) {
            return;
        }
        emit errorOccurred(m_port->errorString());
    });
}

SerialPortManager::~SerialPortManager()
{
    if (m_port && m_port->isOpen()) {
        m_port->close();
    }
}

QVariantList SerialPortManager::availablePorts()
{
    QVariantList out;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        QVariantMap item;
        item.insert(QStringLiteral("portName"), info.portName());
        item.insert(QStringLiteral("description"), info.description());
        item.insert(QStringLiteral("manufacturer"), info.manufacturer());
        out.append(item);
    }
    return out;
}

void SerialPortManager::setPortName(const QString &name) { m_portName = name; }
void SerialPortManager::setBaudRate(qint32 baud) { m_baud = baud; }
void SerialPortManager::setDataBits(int bits) { m_dataBits = bits; }
void SerialPortManager::setParity(int parityIndex) { m_parityIndex = parityIndex; }
void SerialPortManager::setStopBits(int stopIndex) { m_stopIndex = stopIndex; }
void SerialPortManager::setFlowControl(int flowIndex) { m_flowIndex = flowIndex; }

QString SerialPortManager::portName() const { return m_portName; }
qint32 SerialPortManager::baudRate() const { return m_baud; }
int SerialPortManager::dataBits() const { return m_dataBits; }
int SerialPortManager::parityIndex() const { return m_parityIndex; }
int SerialPortManager::stopBitsIndex() const { return m_stopIndex; }
int SerialPortManager::flowControlIndex() const { return m_flowIndex; }

namespace {
QSerialPort::DataBits toDataBits(int v)
{
    switch (v) {
    case 5: return QSerialPort::Data5;
    case 6: return QSerialPort::Data6;
    case 7: return QSerialPort::Data7;
    default: return QSerialPort::Data8;
    }
}
QSerialPort::Parity toParity(int idx)
{
    switch (idx) {
    case 1: return QSerialPort::EvenParity;
    case 2: return QSerialPort::OddParity;
    case 3: return QSerialPort::SpaceParity;
    case 4: return QSerialPort::MarkParity;
    default: return QSerialPort::NoParity;
    }
}
QSerialPort::StopBits toStopBits(int idx)
{
    switch (idx) {
    case 1: return QSerialPort::OneAndHalfStop;
    case 2: return QSerialPort::TwoStop;
    default: return QSerialPort::OneStop;
    }
}
QSerialPort::FlowControl toFlowControl(int idx)
{
    // Qt 仅提供 No/Hardware/Software 三种；索引 3（全部）映射为 Hardware。
    switch (idx) {
    case 1:
    case 3:
        return QSerialPort::HardwareControl;
    case 2: return QSerialPort::SoftwareControl;
    default: return QSerialPort::NoFlowControl;
    }
}
} // namespace

bool SerialPortManager::openPort()
{
    if (m_port->isOpen()) {
        return true;
    }
    if (m_portName.isEmpty()) {
        emit errorOccurred(QStringLiteral("未选择串口。"));
        return false;
    }
    m_port->setPortName(m_portName);
    m_port->setBaudRate(m_baud);
    m_port->setDataBits(toDataBits(m_dataBits));
    m_port->setParity(toParity(m_parityIndex));
    m_port->setStopBits(toStopBits(m_stopIndex));
    m_port->setFlowControl(toFlowControl(m_flowIndex));

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QStringLiteral("打开串口失败：%1").arg(m_port->errorString()));
        return false;
    }
    emit portOpened();
    return true;
}

void SerialPortManager::closePort()
{
    if (m_port->isOpen()) {
        m_port->close();
        emit portClosed();
    }
}

bool SerialPortManager::isOpen() const
{
    return m_port && m_port->isOpen();
}

qint64 SerialPortManager::sendData(const QByteArray &data)
{
    if (!m_port->isOpen() || data.isEmpty()) {
        return -1;
    }
    const qint64 n = m_port->write(data);
    if (n > 0) {
        m_port->flush();
    }
    return n;
}

QByteArray SerialPortManager::readAllAvailable()
{
    return m_port->readAll();
}
