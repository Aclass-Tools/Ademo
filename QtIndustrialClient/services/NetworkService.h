#pragma once

#include <functional>

#include <QTcpSocket>

#include "services/ProtocolParser.h"

class NetworkService : public QObject
{
public:
	explicit NetworkService(QObject* parent = nullptr);

	void connectToHost(const QString& host, quint16 port);
	void disconnectFromHost();
	bool isConnected() const;

	bool sendFrame(const QByteArray& frame, QString* errorMessage = nullptr);
	void setLogCallback(std::function<void(const QString&)> callback);
	void setFrameReceivedCallback(std::function<void(const QByteArray&)> callback);

private:
	void handleReadyRead();
	void log(const QString& message) const;

	QTcpSocket m_socket;
	ProtocolParser m_parser;
	std::function<void(const QString&)> m_logCallback;
	std::function<void(const QByteArray&)> m_frameReceivedCallback;
};
