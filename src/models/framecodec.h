// 帧编解码器（FrameCodec）
// -------------------------
// 根据 BinProtocol 字段表，把“字段名 → 值”映射打包成字节流，
// 或把收到的字节流解成字段值表。
//
// 纯静态工具，无状态。
// - 多字节整数按小端处理。
// - 位字段按“字节内低位优先 + 支持跨字节”解码。
// - Struct 字段递归（本轮支持一层），子字段键为 "父名.子名"。

#pragma once

#include "binprotocolformat.h"

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVariant>

class FrameCodec
{
public:
    // 组帧：返回原始字节流。帧长度取 protocol.frameSize()。
    // 缺失字段按 0 填充；值类型不匹配时尽量强转。
    static QByteArray encode(const BinProtocol &proto,
                             const QMap<QString, QVariant> &values);

    // 解帧：返回 字段名(含结构点号) → 值 的映射。
    // frame 长度不足时，缺失字节按 0 处理。
    static QMap<QString, QVariant> decode(const BinProtocol &proto,
                                          const QByteArray &frame);

    // 便捷：把单个字段值转成可读字符串（hex / 数值）。
    static QString valueToString(const FieldDef &field, const QVariant &value);

    // 便捷：把字节流渲染成 "AA BB CC" 形式的 hex 字符串。
    static QString toHexDisplay(const QByteArray &data, const QString &sep = QStringLiteral(" "));

    // 便捷：把 "AA BB" / "AABB" 形式的 hex 字符串解析为字节流；失败返回空。
    static QByteArray fromHexDisplay(const QString &hex);
};
