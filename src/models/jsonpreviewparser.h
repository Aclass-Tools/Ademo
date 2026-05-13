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
#include <QString>
#include <QStringList>

class JsonPreviewParser
{
public:
    // 加载本地 JSON 到内部缓存。
    bool load(const QString &filePath);
    // 基于内部缓存返回项目列表。
    QStringList projectNames() const;
    // 基于内部缓存按“项目显示名”返回该项目的富文本（HTML）预览；失败时返回空字符串。
    QString formattedTextForProject(const QString &projectDisplayName);

private:
    QJsonObject m_rootObject;
    QJsonArray m_instances;
    QStringList m_projectNames;
    bool m_loaded = false;
};
