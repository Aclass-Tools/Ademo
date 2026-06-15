#include "networkservice.h"

#include <QTcpSocket>

namespace services {

NetworkService::NetworkService(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        const QByteArray data = m_socket->readAll();
        emit rawBytesReceived(data);
        m_parser.appendData(data);
        const auto frames = m_parser.takeAvailableFrames();
        for (const QByteArray &f : frames) {
            emit frameReceived(f);
        }
    });
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        emit connectionStateChanged(true);
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        emit connectionStateChanged(false);
    });
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
#else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [this](QAbstractSocket::SocketError) {
#endif
        emit errorOccurred(m_socket->errorString());
    });
}

NetworkService::~NetworkService() = default;

void NetworkService::connectToHost(const QString &host, quint16 port)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->connectToHost(host, port);
}

void NetworkService::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

bool NetworkService::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool NetworkService::sendFrame(const QByteArray &frame, QString *error)
{
    if (!isConnected()) {
        if (error) *error = QStringLiteral("网口未连接");
        return false;
    }
    const qint64 n = m_socket->write(frame);
    if (n < 0) {
        if (error) *error = QStringLiteral("网口发送失败：%1").arg(m_socket->errorString());
        return false;
    }
    m_socket->flush();
    return true;
}

} // namespace services
