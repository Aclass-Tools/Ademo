// 连接管理器（ConnectionManager）
// --------------------------------
// 全局唯一的连接入口：MainWindow 持有单例，所有页面共用。
//
// 设计要点（借鉴 QtIndustrialClient，但用 Qt 信号槽）：
// - 任意时刻只有一个活动 Transport：串口 或 网口。
// - 各页面订阅本类的 frameReceived（拆好的帧）/ rawBytesReceived（裸字节），
//   不再各自持有 SerialPortManager，彻底解决重复打开串口、配置不一致的问题。
// - TerminalPage 用 rawBytesReceived，协议页用 frameReceived，互不干扰。
// - Modbus 模式当前预留接口（ModbusService 在 Step8 接入）。

#pragma once

#include <QObject>
#include <QString>

namespace services {

class SerialService;
class NetworkService;
class ModbusService;

// 传输方式。
enum class TransportMode {
    None,
    Serial,
    Network,
    Modbus  // 预留
};

// 串口连接配置（UI 收集后传给 connect()）。
struct SerialConfig {
    QString portName;
    qint32 baudRate = 115200;
    int dataBits = 8;        // 5/6/7/8
    int parityIndex = 0;     // 0:无 1:偶 2:奇 3:空号 4:标记
    int stopIndex = 0;       // 0:1 1:1.5 2:2
    int flowIndex = 0;       // 0:无 1:RTS/CTS 2:XON/XOFF 3:全部
};

// 网口连接配置。
struct NetworkConfig {
    QString host = QStringLiteral("192.168.1.100");
    quint16 port = 502;      // Modbus TCP 默认端口
};

// Modbus 连接配置（TCP 或 RTU）。
struct ModbusConfig {
    enum class Link { Tcp, Rtu };
    Link link = Link::Tcp;
    QString host = QStringLiteral("192.168.1.100");  // TCP 用
    quint16 port = 502;                              // TCP 用
    QString portName;                                // RTU 用
    qint32 baudRate = 9600;                          // RTU 用
    int slaveId = 1;                                 // 从站地址
};

class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(QObject *parent = nullptr);

    TransportMode mode() const { return m_mode; }
    bool isConnected() const;

    // 打开串口。
    bool connectSerial(const SerialConfig &cfg, QString *error = nullptr);
    // 打开网口（TCP）。
    bool connectNetwork(const NetworkConfig &cfg, QString *error = nullptr);
    // 打开 Modbus（TCP 或 RTU）。
    bool connectModbus(const ModbusConfig &cfg, QString *error = nullptr);
    // 断开。
    void disconnect();

    // 发送一帧（封装层已组好的完整帧字节）。
    bool sendFrame(const QByteArray &frame, QString *error = nullptr);

    // 发送裸字节（不经协议封装，终端页用）。串口/网口都支持。
    bool sendRaw(const QByteArray &bytes, QString *error = nullptr);

    // 供 UI 枚举串口等场景使用。
    SerialService *serialService() const { return m_serial; }
    NetworkService *networkService() const { return m_network; }
    ModbusService *modbusService() const { return m_modbus; }

signals:
    // 连接状态变化。
    void connectionChanged(bool connected, TransportMode mode);
    // 收到完整帧（协议页用）。
    void frameReceived(const QByteArray &frame);
    // 收到裸字节（终端用）。
    void rawBytesReceived(const QByteArray &bytes);
    // Modbus 读/写结果（寄存器值 + 起始地址）。
    void modbusReplies(const QVector<quint16> &values, quint16 startAddress);
    // 日志（统一日志输出）。
    void log(const QString &message);
    void errorOccurred(const QString &message);

private:
    void switchMode(TransportMode newMode);
    void wireServiceSignals();

    SerialService *m_serial = nullptr;
    NetworkService *m_network = nullptr;
    ModbusService *m_modbus = nullptr;
    TransportMode m_mode = TransportMode::None;
};

} // namespace services
