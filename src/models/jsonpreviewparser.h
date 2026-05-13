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

    // 解析任意 JSON 文本负载：
    // - 成功时返回规范化缩进文本（Indented）
    // - 失败时返回错误信息
    static Result parse(const QByteArray &payload);

    // 从约定结构的 JSON 中提取项目名：
    // - 优先读取 instances[0].project_name
    // - 若存在 project_version，则返回 "name (version)"
    // - 提取失败时返回空字符串
    static QString extractProjectName(const QByteArray &payload);

    // 人类可读的根类型文本。
    static QString rootTypeName(RootType type);
};
