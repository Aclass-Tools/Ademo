#pragma once

#include <QString>
#include <QVector>

#include "models/ParameterRecord.h"

struct ParamSerialComminicate
{
	QString port = QStringLiteral("COM1");
	quint64 baudRate = 460800;
	QString parity = QStringLiteral("NoParity");
	quint64 dataBits = 8;
	QString stopBits = QStringLiteral("OneStop");
	int boardId = 1;
	int parentId = 0;
};

struct ParamNetworkComminicate
{
	QString ip = QStringLiteral("192.168.1.100");
	quint64 port = 502;
};

struct ParamComminicate
{
	QString mode = QStringLiteral("串口");
	ParamSerialComminicate serial;
	ParamNetworkComminicate network;
};

struct ParamConfigure
{
	QVector<ParameterRecord> records;
	ParamComminicate comminicate;
	QString filePath;
};
