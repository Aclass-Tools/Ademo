#include "jsonpreviewparser.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

namespace {
QString nodeIdForPath(const QString &path)
{
    return path.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString scalarToText(const QJsonValue &value)
{
    if (value.isString()) {
        return QStringLiteral("\"%1\"").arg(value.toString().toHtmlEscaped());
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if (value.isNull()) {
        return QStringLiteral("null");
    }
    return QStringLiteral("unknown");
}

QString renderJsonValue(const QJsonValue &value,
                        const QString &path,
                        int depth,
                        const QSet<QString> &expandedNodes)
{
    const QString indent = QStringLiteral("margin-left:%1px;").arg(depth * 18);
    const QString nodeId = nodeIdForPath(path);

    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        const bool isRootObject = (path == QStringLiteral("root"));
        const QString describeText = obj.value(QStringLiteral("describe")).toString().trimmed();
        const QJsonValue instancesValue = obj.value(QStringLiteral("instances"));
        const bool hasDescribe = !describeText.isEmpty();
        const bool hasInstancesCount = instancesValue.isArray();
        QString titleText = describeText;
        if (titleText.isEmpty() && hasInstancesCount) {
            titleText = QStringLiteral("列表");
        }
        if (hasInstancesCount) {
            titleText += QStringLiteral(" (%1)").arg(instancesValue.toArray().size());
        }
        const bool shouldShowHeader = !isRootObject && (hasDescribe || hasInstancesCount);
        // 对于可自描述节点（例如“包含板卡”），默认收起，点击后展开实例内容。
        const bool expanded = shouldShowHeader ? expandedNodes.contains(nodeId) : true;
        QString html;
        if (shouldShowHeader) {
            html += QStringLiteral(
                "<div style='%1'>"
                "<a href='toggle:%2' style='color:#1E40AF;text-decoration:none;font-size:18px;font-weight:600;'>%3 %4</a>"
                "</div>")
                .arg(indent, nodeId,
                     expanded ? QStringLiteral("[-]") : QStringLiteral("[+]"),
                     titleText.toHtmlEscaped());
        }

        if (expanded) {
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                const QString key = it.key();
                // describe 已作为当前对象标题展示，不再在子字段中重复展示。
                if (key == QStringLiteral("describe")) {
                    continue;
                }
                const QString keyLineIndent = QStringLiteral("margin-left:%1px;").arg((depth + 1) * 18);
                const QJsonValue child = it.value();
                // instances 不显示字段名，但要在原位置展示其内容，保持结构顺序自然。
                if (key == QStringLiteral("instances") && child.isArray()) {
                    // 按需求将 instances 内容上提一个层级：与当前对象字段同级显示。
                    html += renderJsonValue(child, path + QStringLiteral(".") + key, depth, expandedNodes);
                    continue;
                }
                // 通用规则：
                // 如果子对象内部已经有 describe/instances（即它能自描述并可折叠），
                // 则不再显示父级字段名（如 boards），直接展示子对象标题（如 包含板卡）。
                if (child.isObject()) {
                    const QJsonObject childObj = child.toObject();
                    const bool childHasDescribe = !childObj.value(QStringLiteral("describe")).toString().trimmed().isEmpty();
                    const bool childHasInstances = childObj.value(QStringLiteral("instances")).isArray();
                    if (childHasDescribe || childHasInstances) {
                        html += renderJsonValue(child, path + QStringLiteral(".") + key, depth + 1, expandedNodes);
                        continue;
                    }
                }
                if (child.isObject() || child.isArray()) {
                    html += QStringLiteral(
                        "<div style='%1'><span style='color:#334155;'>%2</span>:</div>")
                        .arg(keyLineIndent, key.toHtmlEscaped());
                    html += renderJsonValue(child, path + QStringLiteral(".") + key, depth + 2, expandedNodes);
                } else {
                    html += QStringLiteral(
                        "<div style='%1'><span style='color:#334155;'>%2</span>: "
                        "<span style='color:#0F172A;'>%3</span></div>")
                        .arg(keyLineIndent, key.toHtmlEscaped(), scalarToText(child));
                }
            }
        }
        return html;
    }

    if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        QString html;
        // 数组层级不再提供“收起/展开”入口，直接按当前层级展开渲染内容。
        for (int i = 0; i < arr.size(); ++i) {
            const QString rowIndent = QStringLiteral("margin-left:%1px;").arg((depth + 1) * 18);
            const QJsonValue child = arr.at(i);
            if (child.isObject() || child.isArray()) {
                html += renderJsonValue(child,
                    path + QStringLiteral("[") + QString::number(i) + QStringLiteral("]"),
                    depth + 1, expandedNodes);
            } else {
                html += QStringLiteral(
                    "<div style='%1'><span style='color:#334155;'>-</span> "
                    "<span style='color:#0F172A;'>%3</span></div>")
                    .arg(rowIndent, scalarToText(child));
            }
        }
        return html;
    }

    return QStringLiteral("<div style='%1;color:#0F172A;'>%2</div>").arg(indent, scalarToText(value));
}

}

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

JsonPreviewParser::ProjectPreviewResult JsonPreviewParser::formattedTextForProject(
    const QString &projectDisplayName, const QSet<QString> &expandedNodes) const
{
    ProjectPreviewResult result;

    // 未加载数据时无法进行项目过滤，直接返回空字符串。
    // 由上层 UI 决定展示“请先刷新”或其它提示文案。
    if (!m_loaded) {
        return result;
    }

    // 标准化用户选择值：
    // - 去除首尾空白，减少输入噪音；
    // - “未选择项目”属于占位项，不进入解析流程。
    const QString selected = projectDisplayName.trimmed();
    if (selected.isEmpty() || selected == QStringLiteral("未选择项目")) {
        return result;
    }

    // 遍历 instances，按“显示名”匹配目标项目。
    // 命中后直接拼装富文本展示内容，不再返回 JSON 原文。
    QString html;
    html += QStringLiteral(
        "<div style='font-family:\"Microsoft YaHei\",\"Segoe UI\",sans-serif;"
        "font-size:18px;line-height:1.8;color:#0F172A;'>");

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
                "<h3 style='margin:0 0 14px 0;color:#0B1220;font-size:32px;font-weight:800;line-height:1.25;'>%1</h3>")
                .arg(displayText.toHtmlEscaped());
            html += QStringLiteral(
                "<div style='background:#F8FAFC;border:1px solid #CBD5E1;padding:14px;border-radius:8px;'>");
            html += renderJsonValue(QJsonValue(obj), QStringLiteral("root"), 0, expandedNodes);
            html += QStringLiteral("</div>");
        }
    }

    // 没匹配到项目时返回空字符串，让调用方决定提示文案。
    if (!foundAny) {
        return result;
    }

    html += QStringLiteral("</div>");
    result.ok = true;
    result.html = html;
    // 保存顶层 version，供上层做全局版本展示或跨页面共享。
    result.rootVersion = m_rootObject.value(QStringLiteral("version")).toString().trimmed();

    // 从项目对象根层提取 DBconfig.local / DBconfig.remote。
    for (const QJsonValue &item : m_instances) {
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject obj = item.toObject();
        const QString name = obj.value(QStringLiteral("project_name")).toString().trimmed();
        const QString version = obj.value(QStringLiteral("project_version")).toString().trimmed();
        if (name.isEmpty()) {
            continue;
        }
        const QString displayText = version.isEmpty()
            ? name
            : QStringLiteral("%1 (%2)").arg(name, version);
        if (displayText != selected) {
            continue;
        }

        const QJsonObject dbConfig = obj.value(QStringLiteral("DBconfig")).toObject();
        result.localDbAddress = dbConfig.value(QStringLiteral("local")).toString().trimmed();
        result.remoteDbAddress = dbConfig.value(QStringLiteral("remote")).toString().trimmed();
        result.configVersion = obj.value(QStringLiteral("project_version")).toString().trimmed();
        break;
    }

    return result;
}
