#include "modbusservice.h"

#include <QModbusReply>
#include <QModbusRtuSerialClient>
#include <QModbusTcpClient>
#include <QSerialPort>
#include <QVariant>

namespace services {

ModbusService::ModbusService(QObject *parent)
    : QObject(parent)
{
}

ModbusService::~ModbusService()
{
    clearClient();
}

bool ModbusService::connectTcp(const QString &host, quint16 port, int slaveId, QString *error)
{
    clearClient();
    auto *tcp = new QModbusTcpClient(this);
    tcp->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QVariant(host));
    tcp->setConnectionParameter(QModbusDevice::NetworkPortParameter, QVariant(int(port)));
    if (!tcp->connectDevice()) {
        if (error) *error = tcp->errorString();
        tcp->deleteLater();
        return false;
    }
    m_client = tcp;
    m_slaveId = quint8(slaveId);
    emit connectionStateChanged(true);
    return true;
}

bool ModbusService::connectRtu(const QString &portName, qint32 baudRate, int slaveId, QString *error)
{
    clearClient();
    auto *rtu = new QModbusRtuSerialClient(this);
    rtu->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant(portName));
    rtu->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QVariant(int(baudRate)));
    // 默认 8N1。
    rtu->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QVariant(int(QSerialPort::Data8)));
    rtu->setConnectionParameter(QModbusDevice::SerialParityParameter, QVariant(int(QSerialPort::NoParity)));
    rtu->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QVariant(int(QSerialPort::OneStop)));
    rtu->setTimeout(1000);
    if (!rtu->connectDevice()) {
        if (error) *error = rtu->errorString();
        rtu->deleteLater();
        return false;
    }
    m_client = rtu;
    m_slaveId = quint8(slaveId);
    emit connectionStateChanged(true);
    return true;
}

void ModbusService::disconnect()
{
    if (!m_client) {
        return;
    }
    m_client->disconnectDevice();
    clearClient();
    emit connectionStateChanged(false);
}

bool ModbusService::isConnected() const
{
    return m_client && m_client->state() == QModbusDevice::ConnectedState;
}

void ModbusService::readHoldingRegisters(quint16 startAddress, quint16 count)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Modbus 未连接"));
        return;
    }
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, startAddress, count);
    QModbusReply *reply = m_client->sendReadRequest(unit, m_slaveId);
    handleReply(reply, startAddress, QStringLiteral("读保持寄存器"));
}

void ModbusService::readInputRegisters(quint16 startAddress, quint16 count)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Modbus 未连接"));
        return;
    }
    QModbusDataUnit unit(QModbusDataUnit::InputRegisters, startAddress, count);
    QModbusReply *reply = m_client->sendReadRequest(unit, m_slaveId);
    handleReply(reply, startAddress, QStringLiteral("读输入寄存器"));
}

void ModbusService::writeSingleRegister(quint16 address, quint16 value)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Modbus 未连接"));
        return;
    }
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, value);
    QModbusReply *reply = m_client->sendWriteRequest(unit, m_slaveId);
    handleReply(reply, address, QStringLiteral("写单个寄存器"));
}

void ModbusService::writeMultipleRegisters(quint16 startAddress, const QVector<quint16> &values)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Modbus 未连接"));
        return;
    }
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, startAddress, quint16(values.size()));
    for (int i = 0; i < values.size(); ++i) {
        unit.setValue(i, values[i]);
    }
    QModbusReply *reply = m_client->sendWriteRequest(unit, m_slaveId);
    handleReply(reply, startAddress, QStringLiteral("写多个寄存器"));
}

void ModbusService::handleReply(QModbusReply *reply, quint16 startAddress, const QString &action)
{
    if (!reply) {
        if (m_client) {
            emit errorOccurred(QStringLiteral("%1 失败：%2").arg(action, m_client->errorString()));
        } else {
            emit errorOccurred(QStringLiteral("%1 失败：无响应").arg(action));
        }
        return;
    }
    connect(reply, &QModbusReply::finished, this, [this, reply, startAddress, action]() {
        if (reply->error() == QModbusDevice::NoError) {
            const QModbusDataUnit unit = reply->result();
            QVector<quint16> values;
            for (int i = 0; i < int(unit.valueCount()); ++i) {
                values.append(quint16(unit.value(i)));
            }
            emit repliesReceived(values, startAddress);
        } else {
            emit errorOccurred(QStringLiteral("%1 失败：%2").arg(action, reply->errorString()));
        }
        reply->deleteLater();
    });
    connect(reply, &QModbusReply::errorOccurred, this, [this, reply, action](QModbusDevice::Error) {
        emit errorOccurred(QStringLiteral("%1 错误：%2").arg(action, reply->errorString()));
    });
}

void ModbusService::clearClient()
{
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }
}

} // namespace services
