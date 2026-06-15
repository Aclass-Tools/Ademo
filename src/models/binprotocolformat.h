// 自定义二进制协议格式定义（BinProtocolFormat）
// -----------------------------------------------
// 定义 .bin 协议描述文件在内存中的数据结构。
//
// 文件布局（小端）：
//   [BinProtocolHeader]  固定 24 字节
//   [FieldDesc × fieldCount]  每个字段定长描述（见 kFieldDescSize）
//   [名字/备注等字符串]  由字段描述里的偏移指向
//
// 说明：
// - 这里只定义 POD 结构与枚举，不包含任何解析/序列化逻辑。
// - 解析在 BinProtocolLoader 中实现，编解码在 FrameCodec 中实现。
// - 字段定义面向“按字段表自动组帧/解帧”，因此每个字段携带
//   帧内字节偏移与（位字段专用）位偏移/位长度。

#pragma once

#include <QString>
#include <QVector>

// 字段值类型。
// 覆盖整数、浮点、字节数组、位字段、字符串、嵌套结构（本轮支持一层）。
enum class FieldType : quint8 {
    UInt8 = 0,
    UInt16,
    UInt32,
    Int8,
    Int16,
    Int32,
    Float32,
    Float64,
    ByteArray,  // 定长字节数组，长度由 byteLength 决定
    BitField,   // 位字段，bitOffset/bitLength 决定取位范围
    String,     // 定长字符串，长度由 byteLength 决定
    Struct      // 嵌套结构，children 列出成员（本轮只支持一层）
};

// 单个字段定义。
// POD 风格，便于在表格模型中直接展示与编辑。
struct FieldDef {
    QString name;            // 字段名
    FieldType type = FieldType::UInt8;
    quint32 byteOffset = 0;  // 在帧中的字节偏移
    quint16 byteLength = 0;  // 字节长度（位字段也填，通常为其覆盖的字节数）
    quint8 bitOffset = 0;    // 位字段：在覆盖区间内的起始位（0 起，低位在前）
    quint8 bitLength = 0;    // 位字段：位数
    QString comment;         // 备注
    // Struct 子字段（仅当 type == Struct 时有意义）。
    // 本轮支持一层嵌套；子字段的 byteOffset 是相对于父字段起点的偏移。
    QVector<FieldDef> children;
};

// 传输封装协议类型（.bin 协议自带，决定 payload 外面用什么帧封装）。
// 与 A0/A3 帧封装、Modbus 三种传输对应。
// 持久化时编码进 flags 的低 2 位（见 binprotocolloader）。
enum class TransportProtocol : quint8 {
    A0 = 0,
    A3 = 1,
    Modbus = 2,
};

// 整个协议描述。
struct BinProtocol {
    // 文件头字段（内存镜像，便于编辑/回写）。
    QString magic;            // 约定 "APROTO01"
    quint16 version = 1;      // 格式版本
    quint16 flags = 0;        // 低 2 位编码 transport，其余预留
    QVector<FieldDef> fields; // 顶层字段表
    QString sourcePath;       // 来源文件路径（内存中可空）
    // 传输封装协议（内存态；load/save 时与 flags 低 2 位互转）。
    TransportProtocol transport = TransportProtocol::A0;

    // 计算整帧（不含嵌套展开）的最小字节数：取所有顶层字段末尾的最大值。
    // Struct 字段按自身 byteLength 计入。
    quint32 frameSize() const;
};

// 文件头固定大小（字节）。
constexpr quint32 kBinProtocolHeaderSize = 24;
// 单个字段描述在文件中的定长字节数（不含字符串数据）。
//   名称长度(2) + 类型(1) + byteOffset(4) + byteLength(2)
//   + bitOffset(1) + bitLength(1) + 子字段数(2) + 预留(1)
//   + 名字字符串偏移(4) + 备注字符串偏移(4)
// 字符串集中存放在字段表之后，按偏移引用。
constexpr quint32 kFieldDescSize = 24;
