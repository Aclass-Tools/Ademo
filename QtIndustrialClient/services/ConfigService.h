#pragma once

#include <QJsonObject>
#include <QReadWriteLock>
#include <QString>
#include <QVector>

#include "models/ParamConfigure.h"
#include "models/ParameterRecord.h"

class ConfigService
{
public:
	static ConfigService& instance();

	bool loadConfigure(const QString& filePath, ParamConfigure* configure, QString* errorMessage = nullptr);
	bool saveConfigure(const QString& filePath, const ParamConfigure& configure, QString* errorMessage = nullptr);
	ParamConfigure currentConfigure() const;
	void setCurrentConfigure(const ParamConfigure& configure);
	ParamComminicate defaultComminicate() const;
	QJsonObject comminicateToJson(const ParamComminicate& comminicate) const;
	ParamComminicate comminicateFromJson(const QJsonObject& comminicateObject) const;

	QVector<ParameterRecord> loadRecords(const QString& filePath, QString* errorMessage = nullptr);
	bool saveRecords(const QString& filePath, const QVector<ParameterRecord>& records, QString* errorMessage = nullptr);
	QString findDefaultConfigPath() const;

	ConfigService(const ConfigService&)            = delete;
	ConfigService& operator=(const ConfigService&) = delete;

private:
	ConfigService() = default;

	mutable QReadWriteLock m_configLock;
	ParamConfigure         m_currentConfigure;
};
