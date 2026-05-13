// JSON 预览解析器（JsonPreviewParser）
// ------------------------------------
// 将原始 JSON 负载解析为可直接展示的只读文本与元信息。
//
// 说明：
// - 不依赖任何 UI 组件。
// - 仅负责解析与格式化，不负责网络请求与页面渲染。

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

class JsonPreviewParser
{
public:
    enum class RootType {
        Invalid,
        Object,
        Array,
        Primitive
    };

    struct Result {
        bool ok = false;
        RootType rootType = RootType::Invalid;
        QString formattedText;
        QString errorText;
    };

    struct FileLoadResult {
        bool ok = false;
        QByteArray payload;
        QString errorText;
    };

    // 加载本地 JSON 到内部缓存。
    bool load(const QString &filePath);
    // 当前是否已加载有效 JSON。
    bool isLoaded() const;
    // 最近一次加载/解析的错误信息。
    QString lastError() const;
    // 基于内部缓存返回项目列表。
    QStringList projectNames() const;
    // 基于内部缓存返回格式化 JSON 文本；失败时返回空字符串。
    QString formattedText();
    // 基于内部缓存返回当前项目显示名。
    QString currentProjectName() const;

    // 解析任意 JSON 文本负载：
    // - 成功时返回规范化缩进文本（Indented）
    // - 失败时返回错误信息
    static Result parse(const QByteArray &payload);

    // 从本地文件读取 JSON 原始字节流：
    // - 成功时 ok=true，payload 为文件内容
    // - 失败时 ok=false，errorText 为失败原因
    static FileLoadResult loadJsonFile(const QString &filePath);

    // 从约定结构的 JSON 中提取项目名：
    // - 优先读取 instances[0].project_name
    // - 若存在 project_version，则返回 "name (version)"
    // - 提取失败时返回空字符串
    static QString extractProjectName(const QByteArray &payload);

    // 从约定结构的 JSON 中提取全部项目名（去重后按出现顺序返回）：
    // - 遍历 instances[]，读取 project_name / project_version
    // - 若存在 project_version，则返回 "name (version)"
    // - 仅返回非空项目名；重复项仅保留第一次出现
    static QStringList extractProjectNames(const QByteArray &payload);

    // 人类可读的根类型文本。
    static QString rootTypeName(RootType type);

private:
    QByteArray m_payload;
    bool m_loaded = false;
    QString m_lastError;
};
