#include "connectionmanager.h"

#include "modbusservice.h"
#include "networkservice.h"
#include "serialservice.h"

namespace services {

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
    , m_serial(new SerialService(this))
    , m_network(new NetworkService(this))
    , m_modbus(new ModbusService(this))
{
    wireServiceSignals();
}

bool ConnectionManager::isConnected() const
{
    switch (m_mode) {
    case TransportMode::Serial:
        return m_serial->isOpen();
    case TransportMode::Network:
        return m_network->isConnected();
    case TransportMode::Modbus:
        return m_modbus->isConnected();
    case TransportMode::None:
        return false;
    }
    return false;
}

bool ConnectionManager::connectSerial(const SerialConfig &cfg, QString *error)
{
    // 先断开可能存在的其它连接。
    if (isConnected()) {
        disconnect();
    }
    if (!m_serial->openPort(cfg.portName, cfg.baudRate, cfg.dataBits,
                            cfg.parityIndex, cfg.stopIndex, cfg.flowIndex, error)) {
        switchMode(TransportMode::None);
        return false;
    }
    switchMode(TransportMode::Serial);
    emit log(QStringLiteral("串口已连接：%1 @ %2").arg(cfg.portName).arg(cfg.baudRate));
    emit connectionChanged(true, m_mode);
    return true;
}

bool ConnectionManager::connectNetwork(const NetworkConfig &cfg, QString *error)
{
    if (isConnected()) {
        disconnect();
    }
    // TCP 异步连接，这里先发起；真正连上由 connectionStateChanged 信号通知。
    switchMode(TransportMode::Network);
    m_network->connectToHost(cfg.host, cfg.port);
    emit log(QStringLiteral("正在连接网口：%1:%2").arg(cfg.host).arg(cfg.port));
    return true;
}

bool ConnectionManager::connectModbus(const ModbusConfig &cfg, QString *error)
{
    if (isConnected()) {
        disconnect();
    }
    bool ok = false;
    if (cfg.link == ModbusConfig::Link::Tcp) {
        ok = m_modbus->connectTcp(cfg.host, cfg.port, cfg.slaveId, error);
        if (ok) {
            emit log(QStringLiteral("Modbus TCP 已连接：%1:%2（从站 %3）")
                        .arg(cfg.host).arg(cfg.port).arg(cfg.slaveId));
        }
    } else {
        ok = m_modbus->connectRtu(cfg.portName, cfg.baudRate, cfg.slaveId, error);
        if (ok) {
            emit log(QStringLiteral("Modbus RTU 已连接：%1 @ %2（从站 %3）")
                        .arg(cfg.portName).arg(cfg.baudRate).arg(cfg.slaveId));
        }
    }
    if (!ok) {
        switchMode(TransportMode::None);
        return false;
    }
    switchMode(TransportMode::Modbus);
    emit connectionChanged(true, m_mode);
    return true;
}

void ConnectionManager::disconnect()
{
    const TransportMode old = m_mode;
    switch (old) {
    case TransportMode::Serial:
        m_serial->closePort();
        break;
    case TransportMode::Network:
        m_network->disconnectFromHost();
        break;
    case TransportMode::Modbus:
        m_modbus->disconnect();
        break;
    case TransportMode::None:
        return;
    }
    m_mode = TransportMode::None;
    emit connectionChanged(false, old);
}

bool ConnectionManager::sendFrame(const QByteArray &frame, QString *error)
{
    switch (m_mode) {
    case TransportMode::Serial:
        return m_serial->sendFrame(frame, error);
    case TransportMode::Network:
        return m_network->sendFrame(frame, error);
    case TransportMode::Modbus:
        // Modbus 不走字节帧接口；请直接调 modbusService()->read/write 方法。
        if (error) *error = QStringLiteral("Modbus 模式请使用 modbusService 的读/写方法");
        return false;
    case TransportMode::None:
        if (error) *error = QStringLiteral("未连接");
        return false;
    }
    return false;
}

void ConnectionManager::switchMode(TransportMode newMode)
{
    m_mode = newMode;
}

bool ConnectionManager::sendRaw(const QByteArray &bytes, QString *error)
{
    // 裸字节与帧字节在传输层都是"写字节"，直接复用 sendFrame 的路由。
    return sendFrame(bytes, error);
}

void ConnectionManager::wireServiceSignals()
{
    // 两个 service 的信号都透传出去；页面只关心数据，不关心来自哪个 service。
    connect(m_serial, &SerialService::frameReceived, this, &ConnectionManager::frameReceived);
    connect(m_serial, &SerialService::rawBytesReceived, this, &ConnectionManager::rawBytesReceived);
    connect(m_serial, &SerialService::errorOccurred, this, [this](const QString &msg) {
        emit log(msg);
        emit errorOccurred(msg);
    });
    connect(m_serial, &SerialService::portClosed, this, [this]() {
        if (m_mode == TransportMode::Serial) {
            m_mode = TransportMode::None;
            emit connectionChanged(false, TransportMode::Serial);
        }
    });

    connect(m_network, &NetworkService::frameReceived, this, &ConnectionManager::frameReceived);
    connect(m_network, &NetworkService::rawBytesReceived, this, &ConnectionManager::rawBytesReceived);
    connect(m_network, &NetworkService::connectionStateChanged, this, [this](bool connected) {
        // 网口连接由底层异步通知，这里把状态变化转发给 UI。
        if (!connected && m_mode == TransportMode::Network) {
            m_mode = TransportMode::None;
        }
        emit connectionChanged(connected, TransportMode::Network);
    });
    connect(m_network, &NetworkService::errorOccurred, this, [this](const QString &msg) {
        emit log(msg);
        emit errorOccurred(msg);
    });

    // Modbus 信号透传。
    connect(m_modbus, &ModbusService::repliesReceived, this, &ConnectionManager::modbusReplies);
    connect(m_modbus, &ModbusService::connectionStateChanged, this, [this](bool connected) {
        if (!connected && m_mode == TransportMode::Modbus) {
            m_mode = TransportMode::None;
        }
        emit connectionChanged(connected, TransportMode::Modbus);
    });
    connect(m_modbus, &ModbusService::errorOccurred, this, [this](const QString &msg) {
        emit log(msg);
        emit errorOccurred(msg);
    });
}

} // namespace services
