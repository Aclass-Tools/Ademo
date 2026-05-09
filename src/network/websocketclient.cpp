#include "websocketclient.h"

#include <QWebSocket>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    // 默认本地后端 WS 地址。
    , m_url(QStringLiteral("ws://127.0.0.1:8000/ws"))
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    // 重新暴露信号，避免上层直接依赖 QWebSocket。
    connect(m_socket, &QWebSocket::connected, this, &WebSocketClient::connected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocketClient::disconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::textMessageReceived);
    connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });
}

void WebSocketClient::setUrl(const QUrl &url)
{
    m_url = url;
}

QUrl WebSocketClient::url() const
{
    return m_url;
}

void WebSocketClient::connectToServer()
{
    // open 的幂等行为由 QWebSocket 内部处理。
    m_socket->open(m_url);
}

void WebSocketClient::disconnectFromServer()
{
    // 关闭策略交给调用层决定（是否重连等）。
    m_socket->close();
}
