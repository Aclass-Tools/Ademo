// 串口服务（SerialService）
// ---------------------------
// 在 SerialPortManager 之上叠加帧拆包：把裸字节流交给 ProtocolParser，
// 拆出完整 A0/A3 帧后以 frameReceived 信号上抛。
//
// 与 TerminalPage 的关系：
// - TerminalPage 要的是"裸字节流"（直接显示 hex/ascii），订阅 rawBytesReceived。
// - 协议页要的是"拆好的帧"，订阅 frameReceived。
// 两个信号都由本服务同时发出，互不干扰。
//
// 复用 SerialPortManager 的全部串口配置能力，不重新封装 QSerialPort。

#pragma once

#include "network/serialportmanager.h"
#include "services/protocolparser.h"

#include <QObject>

namespace services {

class SerialService : public QObject
{
    Q_OBJECT

public:
    explicit SerialService(QObject *parent = nullptr);

    // 打开串口（内部走 SerialPortManager）。失败返回 false，error 可带出。
    bool openPort(const QString &portName, qint32 baudRate, int dataBits = 8,
                  int parityIndex = 0, int stopIndex = 0, int flowIndex = 0,
                  QString *error = nullptr);
    void closePort();
    bool isOpen() const;

    // 发送一帧（已是封装好的完整帧字节）。
    bool sendFrame(const QByteArray &frame, QString *error = nullptr);

    // 当前串口配置（供 UI 回显）。
    SerialPortManager *manager() { return &m_serial; }

signals:
    // 收到完整帧（经 ProtocolParser 拆包）。
    void frameReceived(const QByteArray &frame);
    // 收到裸字节（不经拆包，给终端/原始显示用）。
    void rawBytesReceived(const QByteArray &bytes);
    void portOpened();
    void portClosed();
    void errorOccurred(const QString &message);

private:
    SerialPortManager m_serial;     // 复用现有串口管理器
    ProtocolParser m_parser;        // 帧拆包缓冲
};

} // namespace services
