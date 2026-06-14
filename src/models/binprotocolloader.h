// 二进制协议加载器（BinProtocolLoader）
// --------------------------------------
// 将本地 .bin 协议描述文件解析为 BinProtocol 内存结构。
//
// 设计参考 JsonPreviewParser：
// - 纯模型层，不依赖 UI。
// - load 阶段一次性解析，后续接口只消费缓存。
// - 失败时清空缓存并返回 false，避免上层误用历史数据。
// - 额外提供 save() 用于协议编辑页回写文件。

#pragma once

#include "binprotocolformat.h"

#include <QString>

class BinProtocolLoader
{
public:
    BinProtocolLoader();

    // 加载本地 .bin 协议描述文件。
    // 成功后可通过 protocol() 获取解析结果。
    bool load(const QString &filePath);

    // 把当前 protocol 写回 .bin 文件（供协议编辑页）。
    bool save(const QString &filePath) const;

    // 直接设置内存中的协议（编辑页“新建”时使用）。
    void setProtocol(const BinProtocol &proto);

    const BinProtocol &protocol() const;
    bool isLoaded() const;
    QString lastError() const;

    // 生成一份覆盖各类型的示例协议，便于演示与测试。
    static BinProtocol makeSample();

private:
    BinProtocol m_protocol;
    bool m_loaded = false;
    QString m_lastError;
};
