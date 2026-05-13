#include "jsonpreviewparser.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

bool JsonPreviewParser::load(const QString &filePath)
{
    // 直接按只读方式打开本地 JSON 文件。
    // 打开失败时清空缓存并返回 false，避免上层误用历史数据。
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_rootObject = QJsonObject();
        m_instances = QJsonArray();
        m_projectNames.clear();
        m_loaded = false;
        return false;
    }

    // 读取原始字节，并在 load 阶段一次性完成 JSON 解析。
    // 后续接口只消费缓存结构，避免重复 fromJson 解析开销。
    const QByteArray payload = file.readAll();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        m_rootObject = QJsonObject();
        m_instances = QJsonArray();
        m_projectNames.clear();
        m_loaded = false;
        return false;
    }

    // 缓存根对象与 instances 数组，供后续查询直接使用。
    m_rootObject = doc.object();
    m_instances = m_rootObject.value(QStringLiteral("instances")).toArray();

    // 同步构建“去重且保持出现顺序”的项目显示名缓存。
    m_projectNames.clear();
    for (const QJsonValue &item : m_instances) {
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
        if (!m_projectNames.contains(displayText)) {
            m_projectNames.append(displayText);
        }
    }

    m_loaded = true;
    return true;
}

QStringList JsonPreviewParser::projectNames() const
{
    // 防御式判断：未加载时直接返回空列表。
    // 这样调用方可以自然地把“无数据”当作空状态处理，不会崩溃。
    if (!m_loaded) {
        return {};
    }

    // 直接返回 load() 阶段构建好的项目名缓存，避免重复解析。
    return m_projectNames;
}

QString JsonPreviewParser::formattedTextForProject(const QString &projectDisplayName)
{
    // 未加载数据时无法进行项目过滤，直接返回空字符串。
    // 由上层 UI 决定展示“请先刷新”或其它提示文案。
    if (!m_loaded) {
        return QString();
    }

    // 标准化用户选择值：
    // - 去除首尾空白，减少输入噪音；
    // - “未选择项目”属于占位项，不进入解析流程。
    const QString selected = projectDisplayName.trimmed();
    if (selected.isEmpty() || selected == QStringLiteral("未选择项目")) {
        return QString();
    }

    // 遍历 instances，按“显示名”匹配目标项目。
    // 命中后直接拼装富文本展示内容，不再返回 JSON 原文。
    QString html;
    html += QStringLiteral(
        "<div style='font-family:\"Microsoft YaHei\",\"Segoe UI\",sans-serif;"
        "font-size:14px;line-height:1.7;color:#EAF2FF;'>");

    bool foundAny = false;
    for (const QJsonValue &item : m_instances) {
        // 非对象元素跳过，保证健壮性。
        if (!item.isObject()) {
            continue;
        }

        const QJsonObject obj = item.toObject();
        // 读取并标准化项目名/版本号。
        const QString name = obj.value(QStringLiteral("project_name")).toString().trimmed();
        const QString version = obj.value(QStringLiteral("project_version")).toString().trimmed();

        // project_name 是最小必需字段，缺失则忽略当前项。
        if (name.isEmpty()) {
            continue;
        }

        // 生成与下拉框一致的“项目显示名”，用于精确匹配。
        const QString displayText = version.isEmpty()
            ? name
            : QStringLiteral("%1 (%2)").arg(name, version);

        // 命中用户选择项后，按“字段名 + 字段值”构建可读富文本。
        if (displayText == selected) {
            foundAny = true;
            html += QStringLiteral(
                "<h3 style='margin:0 0 8px 0;color:#FFFFFF;'>%1</h3>")
                .arg(displayText.toHtmlEscaped());
            html += QStringLiteral(
                "<table style='border-collapse:collapse;width:100%;"
                "background:#162437;border:1px solid #2E4669;'>");

            const QStringList keys = obj.keys();
            for (const QString &key : keys) {
                const QJsonValue value = obj.value(key);
                QString valueText;
                if (value.isObject()) {
                    valueText = QString::fromUtf8(
                        QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
                } else if (value.isArray()) {
                    valueText = QString::fromUtf8(
                        QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
                } else if (value.isString()) {
                    valueText = value.toString();
                } else if (value.isBool()) {
                    valueText = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
                } else if (value.isDouble()) {
                    valueText = QString::number(value.toDouble());
                } else if (value.isNull()) {
                    valueText = QStringLiteral("null");
                }
                html += QStringLiteral(
                    "<tr>"
                    "<td style='width:32%;padding:6px 8px;border:1px solid #2E4669;"
                    "color:#A9C4E8;vertical-align:top;'>%1</td>"
                    "<td style='padding:6px 8px;border:1px solid #2E4669;"
                    "color:#F3F8FF;word-break:break-all;'>%2</td>"
                    "</tr>")
                    .arg(key.toHtmlEscaped(), valueText.toHtmlEscaped());
            }

            html += QStringLiteral("</table>");
        }
    }

    // 没匹配到项目时返回空字符串，让调用方决定提示文案。
    if (!foundAny) {
        return QString();
    }

    html += QStringLiteral("</div>");
    return html;
}
