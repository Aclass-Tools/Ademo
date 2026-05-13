#include "jsonpreviewparser.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

JsonPreviewParser::Result JsonPreviewParser::parse(const QByteArray &payload)
{
    // 默认返回值：先初始化为失败状态。
    // 这样即使后续流程提前 return，也不会出现未定义字段。
    Result result;

    // Qt 的 JSON 解析器会把错误信息写入 QJsonParseError。
    // 这里显式保留 error，方便调用方拿到可展示的错误文案。
    QJsonParseError error;

    // 将原始字节流解析为 QJsonDocument。
    // payload 可以来自本地文件读取、网络返回、或其它数据源。
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    // 解析失败时：
    // 1) 标记 ok=false
    // 2) 根类型设为 Invalid
    // 3) 返回 Qt 原生错误描述（如语法错误、括号不匹配等）
    // 供 UI 层直接展示给用户。
    if (error.error != QJsonParseError::NoError) {
        result.ok = false;
        result.rootType = RootType::Invalid;
        result.errorText = error.errorString();
        return result;
    }

    // 走到这里说明 JSON 语法合法，先标记成功。
    result.ok = true;

    // 识别根节点类型，便于上层做差异化展示逻辑：
    // - Object：通常是单个配置对象
    // - Array：通常是对象列表
    // - Primitive：数字/字符串/布尔/null 等原子值
    if (doc.isObject()) {
        result.rootType = RootType::Object;
    } else if (doc.isArray()) {
        result.rootType = RootType::Array;
    } else {
        result.rootType = RootType::Primitive;
    }

    // 将解析后的 JSON 统一格式化为“带缩进”的文本：
    // - 更适合只读预览
    // - 减少原始数据的杂乱空格差异
    // - 便于用户快速观察层级结构
    result.formattedText = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    return result;
}

JsonPreviewParser::FileLoadResult JsonPreviewParser::loadJsonFile(const QString &filePath)
{
    FileLoadResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.ok = false;
        result.errorText = QStringLiteral("无法打开文件: %1").arg(filePath);
        return result;
    }

    result.ok = true;
    result.payload = file.readAll();
    return result;
}

QString JsonPreviewParser::extractProjectName(const QByteArray &payload)
{
    // 解析根 JSON；失败时直接返回空值，由调用方决定兜底展示。
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QString();
    }

    // 按约定读取 instances 数组中的第一个有效项目。
    const QJsonArray instances = doc.object().value(QStringLiteral("instances")).toArray();
    for (const QJsonValue &item : instances) {
        if (!item.isObject()) {
            continue;
        }

        const QJsonObject obj = item.toObject();
        const QString projectName = obj.value(QStringLiteral("project_name")).toString().trimmed();
        const QString projectVersion = obj.value(QStringLiteral("project_version")).toString().trimmed();
        if (projectName.isEmpty()) {
            continue;
        }

        return projectVersion.isEmpty()
            ? projectName
            : QStringLiteral("%1 (%2)").arg(projectName, projectVersion);
    }

    return QString();
}

QStringList JsonPreviewParser::extractProjectNames(const QByteArray &payload)
{
    QStringList names;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return names;
    }

    const QJsonArray instances = doc.object().value(QStringLiteral("instances")).toArray();
    for (const QJsonValue &item : instances) {
        if (!item.isObject()) {
            continue;
        }

        const QJsonObject obj = item.toObject();
        const QString projectName = obj.value(QStringLiteral("project_name")).toString().trimmed();
        const QString projectVersion = obj.value(QStringLiteral("project_version")).toString().trimmed();
        if (projectName.isEmpty()) {
            continue;
        }

        const QString displayText = projectVersion.isEmpty()
            ? projectName
            : QStringLiteral("%1 (%2)").arg(projectName, projectVersion);
        if (!names.contains(displayText)) {
            names.append(displayText);
        }
    }

    return names;
}

QString JsonPreviewParser::rootTypeName(RootType type)
{
    // RootType 到可读字符串的映射。
    // 这里返回英文关键字，适合做日志、调试信息、状态栏标识等。
    // 如需中文展示，建议在 UI 层做二次映射，避免模型层耦合界面语言。
    switch (type) {
    case RootType::Object:
        return QStringLiteral("object");
    case RootType::Array:
        return QStringLiteral("array");
    case RootType::Primitive:
        return QStringLiteral("primitive");
    case RootType::Invalid:
    default:
        return QStringLiteral("invalid");
    }
}
