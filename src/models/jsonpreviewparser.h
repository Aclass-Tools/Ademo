// JSON 预览解析器（JsonPreviewParser）
// ------------------------------------
// 将原始 JSON 负载解析为可直接展示的只读文本与元信息。
//
// 说明：
// - 不依赖任何 UI 组件。
// - 仅负责解析与格式化，不负责网络请求与页面渲染。

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QStringList>

class JsonPreviewParser
{
public:
    struct ProjectPreviewResult {
        bool ok = false;
        QString html;
        QString localDbAddress;
        QString remoteDbAddress;
        QString configVersion;
    };

    // 加载本地 JSON 到内部缓存。
    bool load(const QString &filePath);
    // 基于内部缓存返回项目列表。
    QStringList projectNames() const;
    // 基于内部缓存按“项目显示名”返回该项目的结构化预览结果：
    // - html：用于页面展示的富文本
    // - localDbAddress/remoteDbAddress：从 DBconfig 中提取的本地/远程地址
    // expandedNodes 用于控制哪些对象/数组节点处于展开状态（用于 UI 点击展开/收缩）。
    ProjectPreviewResult formattedTextForProject(const QString &projectDisplayName,
                                                 const QSet<QString> &expandedNodes = {}) const;

private:
    QJsonObject m_rootObject;
    QJsonArray m_instances;
    QStringList m_projectNames;
    bool m_loaded = false;
};
