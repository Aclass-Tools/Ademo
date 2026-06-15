/**
 * @file ParameterRecord.h
 * @brief 参数记录数据结构定义
 * @author Claude Code
 * @date 2026-04-10
 */

#pragma once

#include <QString>
#include <QStringList>

/**
 * @struct ParameterRecord
 * @brief 参数记录结构，表示一个可配置的设备参数
 *
 * 包含参数的完整定义信息，包括协议类型、分组信息、
 * 权限、数据类型、验证规则等。
 */
struct ParameterRecord
{
	quint8      protocolType = 0; ///< 协议类型 (0xA0 或 0xA3)
	QString     groupName;        ///< 分组显示名称
	int         groupId = 0;      ///< 分组标识符
	QString     commandName;      ///< 命令名称
	int         commandId = 0;    ///< 命令标识符
	QString     permission;       ///< 权限 ("Read", "Write", "ReadWrite")
	int         readIndex = 0;    ///< 在负载中的偏移位置
	QString     parameterName;    ///< 参数显示名称
	QString     dataType;         ///< 数据类型 (int, float, enum 等)
	int         dataLength = 0;   ///< 数据字节长度
	QString     description;      ///< 参数描述
	QString     defaultValue;     ///< 默认值
	QString     unit;             ///< 单位
	QString     storageLocation;  ///< 存储位置
	QString     maxValue;         ///< 最大值（验证用）
	QString     minValue;         ///< 最小值（验证用）
	QString     step;             ///< 步长（验证用）
	QStringList enumValues;       ///< 枚举值列表（枚举类型用）
	QString     remark;           ///< 备注
};

/**
 * @enum ParameterAction
 * @brief 参数操作类型枚举
 */
enum class ParameterAction
{
	Read, ///< 读取操作
	Write ///< 写入操作
};
