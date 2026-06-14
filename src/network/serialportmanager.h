// 串口管理器（SerialPortManager）
// --------------------------------
// QSerialPort 的轻量封装，参考 WebSocketClient 的封装风格。
//
// 目的：
// - 把 QSerialPort 生命周期与 Qt 原生信号映射隔离开来。
// - 对上层提供稳定的事件面（打开/关闭/收到数据/出错），不暴露 QSerialPort。
// - 提供“枚举可用串口”静态工具。

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QSerialPort;
class QSerialPortInfo;

class SerialPortManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager() override;

    // 枚举可用串口：返回 QVariantList（portName / description / manufacturer），
    // 避免上层直接依赖 QSerialPortInfo。
    static QVariantList availablePorts();

    // 配置（打开前设置）。
    void setPortName(const QString &name);
    void setBaudRate(qint32 baud);
    void setDataBits(int bits);          // 5/6/7/8
    void setParity(int parityIndex);     // 0:无 1:偶 2:奇 3:空号 4:标记
    void setStopBits(int stopIndex);     // 0:1 1:1.5 2:2
    void setFlowControl(int flowIndex);  // 0:无 1:RTS/CTS 2:XON/XOFF 3:全部

    QString portName() const;
    qint32 baudRate() const;
    int dataBits() const;
    int parityIndex() const;
    int stopBitsIndex() const;
    int flowControlIndex() const;

    // 控制。
    bool openPort();
    void closePort();
    bool isOpen() const;

    // 发送数据（同步写）。返回实际写入字节数，-1 表示失败。
    qint64 sendData(const QByteArray &data);

    // 一次性读取并清空接收缓冲区中已到达的数据。
    QByteArray readAllAvailable();

signals:
    // 收到数据（readyRead 触发后异步发出）。
    void dataReceived(const QByteArray &data);
    void portOpened();
    void portClosed();
    void errorOccurred(const QString &message);

private:
    QSerialPort *m_port = nullptr;
    QString m_portName;
    qint32 m_baud = 115200;
    int m_dataBits = 8;
    int m_parityIndex = 0;
    int m_stopIndex = 0;
    int m_flowIndex = 0;
};
