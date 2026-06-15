#include "services/ConfigService.h"
#include "services/ProtocolParser.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace
{
	QString readString(const QJsonObject& object, const char* key)
	{
		return object.value(QLatin1String(key)).toVariant().toString().trimmed();
	}

	int readInt(const QJsonObject& object, const char* key)
	{
		return object.value(QLatin1String(key)).toVariant().toInt();
	}

	QStringList readStringList(const QJsonObject& object, const char* key)
	{
		QStringList      values;
		const QJsonArray array = object.value(QLatin1String(key)).toArray();
		for (const QJsonValue& value : array)
		{
			values.push_back(value.toString());
		}
		return values;
	}

	bool parseProtocolRecords(const QJsonObject& rootObject, QVector<ParameterRecord>* records, QString* errorMessage)
	{
		if (records == nullptr)
		{
			if (errorMessage != nullptr)
			{
				*errorMessage = QStringLiteral("配置接收对象为空");
			}
			return false;
		}

		records->clear();
		const QJsonArray protocols = rootObject.value(QStringLiteral("protocol")).toArray();
		for (const QJsonValue& protocolValue : protocols)
		{
			const QJsonObject protocolObject   = protocolValue.toObject();
			const QString     protocolTypeStr  = readString(protocolObject, "protocol_type");
			const quint8      protocolType     = protocolStringToByte(protocolTypeStr);
			const QJsonArray  groups           = protocolObject.value(QStringLiteral("order")).toArray();

			if (groups.isEmpty())
			{
				ParameterRecord record;
				record.protocolType = protocolType;
				record.groupId      = 0;
				record.commandId    = 0;
				record.description  = QStringLiteral("当前协议下暂无指令");
				records->push_back(record);
				continue;
			}

			for (const QJsonValue& groupValue : groups)
			{
				const QJsonObject groupObject = groupValue.toObject();
				const QString     groupName   = readString(groupObject, "first_name");
				const int         groupId     = readInt(groupObject, "first_ID");
				const QJsonArray  commands    = groupObject.value(QStringLiteral("first_index")).toArray();

				for (const QJsonValue& commandValue : commands)
				{
					const QJsonObject commandObject = commandValue.toObject();
					const QString     commandName   = readString(commandObject, "last_name");
					const int         commandId     = readInt(commandObject, "last_ID");
					const QString     permission    = readString(commandObject, "permission");
					const QString     remark        = readString(commandObject, "remark");
					const QJsonArray  params        = commandObject.value(QStringLiteral("last_index")).toArray();

					if (params.isEmpty())
					{
						ParameterRecord record;
						record.protocolType  = protocolType;
						record.groupName     = groupName;
						record.groupId       = groupId;
						record.commandName   = commandName;
						record.commandId     = commandId;
						record.permission    = permission;
						record.readIndex     = 0;
						record.parameterName = commandName;
						record.description   = QString();
						record.remark        = remark;
						records->push_back(record);
						continue;
					}

					int readIndex = 1;
					for (const QJsonValue& paramValue : params)
					{
						const QJsonObject paramObject = paramValue.toObject();

						ParameterRecord record;
						record.protocolType    = protocolType;
						record.groupName       = groupName;
						record.groupId         = groupId;
						record.commandName     = commandName;
						record.commandId       = commandId;
						record.permission      = permission;
						record.readIndex       = readIndex++;
						record.parameterName   = commandName;
						record.dataType        = readString(paramObject, "type");
						record.dataLength      = readInt(paramObject, "len");
						record.description     = readString(paramObject, "name");
						record.defaultValue    = readString(paramObject, "default_value");
						record.unit            = readString(paramObject, "unit");
						record.storageLocation = readString(paramObject, "save_string");
						record.maxValue        = readString(paramObject, "up_limit");
						record.minValue        = readString(paramObject, "down_limit");
						record.step            = readString(paramObject, "step");
						record.enumValues      = readStringList(paramObject, "enum");
						record.remark          = remark;
						records->push_back(record);
					}
				}
			}
		}

		if (errorMessage != nullptr)
		{
			errorMessage->clear();
		}
		return true;
	}

	QJsonObject buildProtocolRootObject(const QVector<ParameterRecord>& records)
	{
		QMap<quint8, QMap<int, QMap<int, QVector<ParameterRecord>>>> protocolGrouped;
		for (const ParameterRecord& record : records)
		{
			protocolGrouped[record.protocolType][record.groupId][record.commandId].push_back(record);
		}

		QJsonArray protocolArray;
		for (auto itProtocol = protocolGrouped.begin(); itProtocol != protocolGrouped.end(); ++itProtocol)
		{
			QJsonObject protocolObject;
			protocolObject["protocol_type"] = protocolByteToString(itProtocol.key());

			QJsonArray groupsArray;
			auto&      groupMap = itProtocol.value();

			for (auto itGroup = groupMap.begin(); itGroup != groupMap.end(); ++itGroup)
			{
				QJsonObject groupObject;
				const int   groupId = itGroup.key();

				QString groupName;
				if (!itGroup.value().isEmpty() && !itGroup.value().begin().value().isEmpty())
				{
					groupName = itGroup.value().begin().value().first().groupName;
				}

				groupObject["first_name"] = groupName;
				groupObject["first_ID"]   = groupId;

				QJsonArray commandsArray;
				auto&      commandMap = itGroup.value();
				for (auto itCommand = commandMap.begin(); itCommand != commandMap.end(); ++itCommand)
				{
					QJsonObject commandObject;
					const int   commandId = itCommand.key();

					if (!itCommand.value().isEmpty())
					{
						const ParameterRecord& first = itCommand.value().first();
						commandObject["last_name"]  = first.parameterName;
						commandObject["last_ID"]    = commandId;
						commandObject["permission"] = first.permission;
						commandObject["remark"]     = first.remark;
					}

					QJsonArray paramsArray;
					for (const ParameterRecord& record : itCommand.value())
					{
						QJsonObject paramObject;
						paramObject["name"]          = record.description;
						paramObject["type"]          = record.dataType;
						paramObject["len"]           = record.dataLength;
						paramObject["default_value"] = record.defaultValue;
						paramObject["unit"]          = record.unit;
						paramObject["save_string"]   = record.storageLocation;
						paramObject["up_limit"]      = record.maxValue;
						paramObject["down_limit"]    = record.minValue;
						paramObject["step"]          = record.step;

						QJsonArray enumArray;
						for (const QString& enumValue : record.enumValues)
						{
							enumArray.append(enumValue);
						}
						paramObject["enum"] = enumArray;
						paramsArray.append(paramObject);
					}

					commandObject["last_index"] = paramsArray;
					commandsArray.append(commandObject);
				}

				groupObject["first_index"] = commandsArray;
				groupsArray.append(groupObject);
			}

			protocolObject["order"] = groupsArray;
			protocolArray.append(protocolObject);
		}

		QJsonObject rootObject;
		rootObject["protocol"] = protocolArray;
		return rootObject;
	}
}

ConfigService& ConfigService::instance()
{
	static ConfigService service;
	return service;
}

ParamComminicate ConfigService::defaultComminicate() const
{
	return ParamComminicate{};
}

QJsonObject ConfigService::comminicateToJson(const ParamComminicate& comminicate) const
{
	QJsonObject commObject;
	commObject["mode"] = comminicate.mode;

	QJsonObject serialObject;
	serialObject["port"]     = comminicate.serial.port;
	serialObject["baudRate"] = static_cast<qint64>(comminicate.serial.baudRate);
	serialObject["parity"]   = comminicate.serial.parity;
	serialObject["dataBits"] = static_cast<qint64>(comminicate.serial.dataBits);
	serialObject["stopBits"] = comminicate.serial.stopBits;
	serialObject["boardId"]  = comminicate.serial.boardId;
	serialObject["parentId"] = comminicate.serial.parentId;
	commObject["serial"]     = serialObject;

	QJsonObject networkObject;
	networkObject["ip"]   = comminicate.network.ip;
	networkObject["port"] = static_cast<qint64>(comminicate.network.port);
	commObject["network"] = networkObject;
	return commObject;
}

ParamComminicate ConfigService::comminicateFromJson(const QJsonObject& comminicateObject) const
{
	ParamComminicate result = defaultComminicate();
	if (comminicateObject.isEmpty())
	{
		return result;
	}

	result.mode = comminicateObject.value("mode").toString(result.mode);

	const QJsonObject serialObject = comminicateObject.value("serial").toObject();
	result.serial.port             = serialObject.value("port").toString(result.serial.port);
	result.serial.baudRate         = static_cast<quint64>(serialObject.value("baudRate").toInteger(result.serial.baudRate));
	result.serial.parity           = serialObject.value("parity").toString(result.serial.parity);
	result.serial.dataBits         = static_cast<quint64>(serialObject.value("dataBits").toInteger(result.serial.dataBits));
	result.serial.stopBits         = serialObject.value("stopBits").toString(result.serial.stopBits);
	result.serial.boardId          = serialObject.value("boardId").toInt(result.serial.boardId);
	result.serial.parentId         = serialObject.value("parentId").toInt(result.serial.parentId);

	const QJsonObject networkObject = comminicateObject.value("network").toObject();
	result.network.ip               = networkObject.value("ip").toString(result.network.ip);
	result.network.port             = static_cast<quint64>(networkObject.value("port").toInteger(result.network.port));
	return result;
}

bool ConfigService::loadConfigure(const QString& filePath, ParamConfigure* configure, QString* errorMessage)
{
	if (configure == nullptr)
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("配置接收对象为空");
		}
		return false;
	}

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("无法打开配置文件: %1").arg(filePath);
		}
		return false;
	}

	QJsonParseError     parseError;
	const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();
	if (parseError.error != QJsonParseError::NoError || !document.isObject())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("配置文件格式错误: %1").arg(parseError.errorString());
		}
		return false;
	}

	const QJsonObject rootObject = document.object();
	QVector<ParameterRecord> records;
	QString parseMessage;
	if (!parseProtocolRecords(rootObject, &records, &parseMessage))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = parseMessage;
		}
		return false;
	}

	ParamConfigure loaded;
	loaded.filePath    = filePath;
	loaded.records     = records;
	loaded.comminicate = comminicateFromJson(rootObject.value(QStringLiteral("comminicate")).toObject());

	{
		QWriteLocker locker(&m_configLock);
		m_currentConfigure = loaded;
	}

	if (configure != nullptr)
	{
		*configure = loaded;
	}
	if (errorMessage != nullptr)
	{
		errorMessage->clear();
	}
	return true;
}

bool ConfigService::saveConfigure(const QString& filePath, const ParamConfigure& configure, QString* errorMessage)
{
	QJsonObject rootObject = buildProtocolRootObject(configure.records);
	const QJsonObject commObject = comminicateToJson(configure.comminicate);
	rootObject["comminicate"] = commObject;

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = QStringLiteral("无法打开配置文件: %1").arg(filePath);
		}
		return false;
	}

	QJsonDocument doc(rootObject);
	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();

	ParamConfigure saved = configure;
	saved.filePath       = filePath;
	saved.comminicate    = comminicateFromJson(commObject);
	{
		QWriteLocker locker(&m_configLock);
		m_currentConfigure = saved;
	}

	if (errorMessage != nullptr)
	{
		errorMessage->clear();
	}
	return true;
}

ParamConfigure ConfigService::currentConfigure() const
{
	QReadLocker locker(&m_configLock);
	return m_currentConfigure;
}

void ConfigService::setCurrentConfigure(const ParamConfigure& configure)
{
	QWriteLocker locker(&m_configLock);
	m_currentConfigure = configure;
}

QVector<ParameterRecord> ConfigService::loadRecords(const QString& filePath, QString* errorMessage)
{
	ParamConfigure configure;
	if (!loadConfigure(filePath, &configure, errorMessage))
	{
		return {};
	}
	return configure.records;
}

bool ConfigService::saveRecords(const QString& filePath, const QVector<ParameterRecord>& records, QString* errorMessage)
{
	ParamConfigure configure = currentConfigure();
	configure.records        = records;
	return saveConfigure(filePath, configure, errorMessage);
}

QString ConfigService::findDefaultConfigPath() const
{
	QDir current(QCoreApplication::applicationDirPath());
	for (int depth = 0; depth < 8; ++depth)
	{
		const QString candidate = current.filePath(QStringLiteral("document/ceg_config.json"));
		if (QFile::exists(candidate))
		{
			return QDir::cleanPath(candidate);
		}
		if (!current.cdUp())
		{
			break;
		}
	}

	return QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../../document/ceg_config.json"));
}
