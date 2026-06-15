#include "services/NetworkService.h"

#include <utility>

#include <QAbstractSocket>

NetworkService::NetworkService(QObject* parent)
	: QObject(parent)
{
	QObject::connect(&m_socket, &QTcpSocket::connected, this, [this]()
	{
		log(QStringLiteral("网口已连接: %1:%2").arg(m_socket.peerAddress().toString()).arg(m_socket.peerPort()));
	});
	QObject::connect(&m_socket, &QTcpSocket::disconnected, this, [this]()
	{
		log(QStringLiteral("网口连接已断开"));
	});
	QObject::connect(&m_socket, &QTcpSocket::readyRead, this, [this]()
	{
		handleReadyRead();
	});
	QObject::connect(&m_socket,
	                 &QTcpSocket::errorOccurred,
	                 this,
	                 [this](QAbstractSocket::SocketError)
	{
		log(QStringLiteral("网口异常: %1").arg(m_socket.errorString()));
	});
}

void NetworkService::connectToHost(const QString& host, quint16 port)
{
	if (m_socket.state() != QAbstractSocket::UnconnectedState)
	{
		m_socket.abort();
	}

	log(QStringLiteral("开始连接网口: %1:%2").arg(host).arg(port));
	m_socket.connectToHost(host, port);
	if (!m_socket.waitForConnected(3000))
	{
		log(QStringLiteral("网口连接失败: %1").arg(m_socket.errorString()));
	}
}

void NetworkService::disconnectFromHost()
{
	if (m_socket.state() == QAbstractSocket::UnconnectedState)
	{
		return;
	}

	m_socket.disconnectFromHost();
	if (m_socket.state() != QAbstractSocket::UnconnectedState)
	{
		m_socket.waitForDisconnected(1000);
	}
	if (m_socket.state() != QAbstractSocket::UnconnectedState)
	{
		m_socket.abort();
	}
}

bool NetworkService::isConnected() const
{
	return m_socket.state() == QAbstractSocket::ConnectedState;
}

bool NetworkService::sendFrame(const QByteArray& frame, QString* errorMessage)
{
	if (!isConnected())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("网口未连接");
		}
		return false;
	}

	if (m_socket.write(frame) < 0)
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = m_socket.errorString();
		}
		log(QStringLiteral("网口发送失败: %1").arg(m_socket.errorString()));
		return false;
	}

	log(QStringLiteral("网口发送 %1 字节").arg(frame.size()));
	return true;
}

void NetworkService::setLogCallback(std::function<void(const QString&)> callback)
{
	m_logCallback = std::move(callback);
}

void NetworkService::setFrameReceivedCallback(std::function<void(const QByteArray&)> callback)
{
	m_frameReceivedCallback = std::move(callback);
}

void NetworkService::handleReadyRead()
{
	m_parser.appendData(m_socket.readAll());
	const QVector<QByteArray> frames = m_parser.takeAvailableFrames();
	for (const QByteArray& frame : frames)
	{
		log(QStringLiteral("网口接收帧 %1 字节").arg(frame.size()));
		if (m_frameReceivedCallback)
		{
			m_frameReceivedCallback(frame);
		}
	}
}

void NetworkService::log(const QString& message) const
{
	if (m_logCallback)
	{
		m_logCallback(message);
	}
}
