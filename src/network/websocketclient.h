// WebSocket 客户端（WebSocketClient）
// --------------------------------------
// QWebSocket 的轻量封装。
//
// 目的：
// - 将 socket 生命周期与 Qt 原生信号映射隔离。
// - 对上层提供稳定事件面，不暴露底层实现细节。
// - 为后续重连/退避策略预留扩展点。

#pragma once

#include <QObject>
#include <QUrl>

class QWebSocket;

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);

    // 运行时可配置 WebSocket 地址。
    void setUrl(const QUrl &url);
    QUrl url() const;

    // 打开/关闭底层 socket。
    void connectToServer();
    void disconnectFromServer();

signals:
    // 连接状态事件透传。
    void connected();
    void disconnected();
    // 文本消息原样输出，解析由调用层负责。
    void textMessageReceived(const QString &message);
    void errorOccurred(const QString &message);

private:
    QUrl m_url;
    // 由本封装类持有。
    QWebSocket *m_socket;
};
