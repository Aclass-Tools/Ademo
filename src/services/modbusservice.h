// Modbus 服务（ModbusService）
// --------------------------------
// 基于 Qt SerialBus（QModbusTcpClient / QModbusRtuSerialMaster）。
// 与 A0/A3 协议不同：Modbus 有自己的 ADU/PDU，不走 ProtocolParser，
// 而是直接用 QtSerialBus 的 read/write API。
//
// 当前实现：
// - 连接：TCP（connectTcp）或 RTU 串口（connectRtu）。
// - 读保持寄存器（FC=0x03）：readHoldingRegisters。
// - 写单个寄存器（FC=0x06）：writeSingleRegister。
// - 写多个寄存器（FC=0x10）：writeMultipleRegisters。
// - 读输入寄存器（FC=0x04）：readInputRegisters。
// 结果通过信号 repliesReceived（寄存器值列表）回传。

#pragma once

#include <QObject>
#include <QVector>
#include <QtSerialBus/QModbusDataUnit>

QT_BEGIN_NAMESPACE
class QModbusClient;
class QModbusReply;
QT_END_NAMESPACE

namespace services {

class ModbusService : public QObject
{
    Q_OBJECT

public:
    explicit ModbusService(QObject *parent = nullptr);
    ~ModbusService() override;

    // TCP 连接。
    bool connectTcp(const QString &host, quint16 port, int slaveId = 1, QString *error = nullptr);
    // RTU 串口连接。
    bool connectRtu(const QString &portName, qint32 baudRate, int slaveId = 1, QString *error = nullptr);
    void disconnect();
    bool isConnected() const;

    // 从站 ID（设备地址）。
    void setSlaveId(int id) { m_slaveId = quint8(id); }
    quint8 slaveId() const { return m_slaveId; }

    // 读保持寄存器（FC=0x03）。count 为寄存器数（每寄存器 2 字节）。
    void readHoldingRegisters(quint16 startAddress, quint16 count);
    // 读输入寄存器（FC=0x04）。
    void readInputRegisters(quint16 startAddress, quint16 count);
    // 写单个寄存器（FC=0x06）。
    void writeSingleRegister(quint16 address, quint16 value);
    // 写多个寄存器（FC=0x10）。
    void writeMultipleRegisters(quint16 startAddress, const QVector<quint16> &values);

signals:
    // 读/写完成，返回寄存器值（写操作时 values 可能为空）。
    void repliesReceived(const QVector<quint16> &values, quint16 startAddress);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString &message);

private:
    void handleReply(QModbusReply *reply, quint16 startAddress, const QString &action);
    void clearClient();

    QModbusClient *m_client = nullptr;
    quint8 m_slaveId = 1;
};

} // namespace services
