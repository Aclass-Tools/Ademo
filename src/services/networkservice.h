// 网口服务（NetworkService）
// ----------------------------
// TCP Socket 传输 + ProtocolParser 帧拆包。
// 接口与 SerialService 对称，便于 ConnectionManager 统一调度。

#pragma once

#include "services/protocolparser.h"

#include <QObject>
#include <QString>

class QTcpSocket;

namespace services {

class NetworkService : public QObject
{
    Q_OBJECT

public:
    explicit NetworkService(QObject *parent = nullptr);
    ~NetworkService() override;

    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();
    bool isConnected() const;

    bool sendFrame(const QByteArray &frame, QString *error = nullptr);

signals:
    void frameReceived(const QByteArray &frame);
    void rawBytesReceived(const QByteArray &bytes);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString &message);

private:
    QTcpSocket *m_socket = nullptr;
    ProtocolParser m_parser;
};

} // namespace services
