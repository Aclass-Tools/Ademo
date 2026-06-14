// BinProtocolLoader 实现。
// 文件布局（全部小端）：
//   1) 头 24 字节：magic(8) + version(2) + flags(2) + fieldCount(4) + payloadOffset(4) + reserved(4)
//   2) 字段描述表：fieldCount × 24 字节（见 kFieldDescSize）
//   3) 子字段描述：仅 Struct 字段，紧接顶层字段表；每个 Struct 的子字段连续存放，
//      由顶层字段描述里的 childCount 决定数量。
//   4) 字符串池：所有字段名/备注/子字段名/备注顺序拼接，各自用 2 字节长度前缀（小端）。
//      字段描述里以“字符串序号 (quint32)”引用，而非字节偏移，便于序列化。

#include "binprotocolloader.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QIODevice>

namespace {

// 字符串池：把每条字符串登记一次，返回序号；相同内容复用同一序号。
class StringPool
{
public:
    quint32 add(const QString &s)
    {
        const QByteArray utf8 = s.toUtf8();
        const int idx = m_entries.indexOf(utf8);
        if (idx >= 0) {
            return quint32(idx);
        }
        m_entries.append(utf8);
        return quint32(m_entries.size() - 1);
    }
    QString at(quint32 idx) const
    {
        return idx < quint32(m_entries.size())
            ? QString::fromUtf8(m_entries.at(int(idx)))
            : QString();
    }
    const QVector<QByteArray> &entries() const { return m_entries; }
    // 仅供加载阶段批量回填用。
    QVector<QByteArray> &mutableEntries() { return m_entries; }

private:
    QVector<QByteArray> m_entries;
};

// 读取小端无符号整数，越界返回 0。
template <typename T>
T readLE(const QByteArray &data, qint64 &pos)
{
    const int sz = int(sizeof(T));
    if (pos + sz > data.size()) {
        pos = data.size();
        return T(0);
    }
    T v = 0;
    for (int i = 0; i < sz; ++i) {
        v |= T(quint8(data.at(int(pos) + i))) << (8 * i);
    }
    pos += sz;
    return v;
}

// 写入小端无符号整数。
template <typename T>
void writeLE(QByteArray &out, T v)
{
    const int sz = int(sizeof(T));
    for (int i = 0; i < sz; ++i) {
        out.append(char((v >> (8 * i)) & 0xFF));
    }
}

// 按字段描述定长（kFieldDescSize 字节）读取一个字段。
// strings 为已加载的字符串池；children 中读取子字段描述（每个也是 kFieldDescSize 字节）。
FieldDef readFieldDesc(const QByteArray &data, qint64 &pos, const StringPool &strings)
{
    FieldDef f;
    const quint32 nameIdx = readLE<quint32>(data, pos);
    f.name = strings.at(nameIdx);
    f.type = FieldType(readLE<quint8>(data, pos));
    f.byteOffset = readLE<quint32>(data, pos);
    f.byteLength = readLE<quint16>(data, pos);
    f.bitOffset = readLE<quint8>(data, pos);
    f.bitLength = readLE<quint8>(data, pos);
    const quint16 childCount = readLE<quint16>(data, pos);
    // 预留 1 字节，保持定长。
    pos += 1;
    const quint32 commentIdx = readLE<quint32>(data, pos);
    f.comment = strings.at(commentIdx);
    // 4 字节对齐填充，使每条字段描述恰好 kFieldDescSize=24 字节，
    // 便于按字段数直接定位字符串池起始。
    pos += 4;
    for (quint16 i = 0; i < childCount; ++i) {
        f.children.append(readFieldDesc(data, pos, strings));
    }
    return f;
}

// 按字段描述定长写入一个字段，同时把字符串登记进 pool。
void writeFieldDesc(QByteArray &out, const FieldDef &f, StringPool &pool)
{
    writeLE<quint32>(out, pool.add(f.name));
    writeLE<quint8>(out, quint8(f.type));
    writeLE<quint32>(out, f.byteOffset);
    writeLE<quint16>(out, f.byteLength);
    writeLE<quint8>(out, f.bitOffset);
    writeLE<quint8>(out, f.bitLength);
    writeLE<quint16>(out, quint16(f.children.size()));
    out.append(char(0)); // 预留
    writeLE<quint32>(out, pool.add(f.comment));
    // 4 字节对齐填充，使本字段描述恰好 kFieldDescSize=24 字节（与 readFieldDesc 对应）。
    out.append(char(0));
    out.append(char(0));
    out.append(char(0));
    out.append(char(0));
    // 递归写入子字段（深度一般 <=1）。填充在前，子字段紧随其后。
    for (const FieldDef &c : f.children) {
        writeFieldDesc(out, c, pool);
    }
}

} // namespace

quint32 BinProtocol::frameSize() const
{
    quint32 tail = 0;
    for (const FieldDef &f : fields) {
        const quint32 end = f.byteOffset + (f.byteLength > 0 ? f.byteLength : 0u);
        if (end > tail) {
            tail = end;
        }
    }
    return tail;
}

BinProtocolLoader::BinProtocolLoader() = default;

bool BinProtocolLoader::load(const QString &filePath)
{
    m_loaded = false;
    m_lastError.clear();
    m_protocol = BinProtocol();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QStringLiteral("无法打开文件：%1").arg(filePath);
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    if (data.size() < int(kBinProtocolHeaderSize)) {
        m_lastError = QStringLiteral("文件过小，不是有效的协议描述文件。");
        return false;
    }

    qint64 pos = 0;
    // 头：magic(8)
    QByteArray magic(8, '\0');
    for (int i = 0; i < 8; ++i) {
        magic[i] = data.at(pos++);
    }
    const QString magicStr = QString::fromLatin1(magic);
    if (magicStr != QStringLiteral("APROTO01")) {
        m_lastError = QStringLiteral("magic 不匹配，期望 APROTO01。");
        return false;
    }
    m_protocol.magic = magicStr;
    m_protocol.version = readLE<quint16>(data, pos);
    m_protocol.flags = readLE<quint16>(data, pos);
    const quint32 fieldCount = readLE<quint32>(data, pos);
    const quint32 payloadOffset = readLE<quint32>(data, pos);
    // header 第 5 个 4 字节字段：字符串池起始偏移。
    const quint32 stringPoolOffset = readLE<quint32>(data, pos);

    if (payloadOffset > quint32(data.size())) {
        m_lastError = QStringLiteral("payloadOffset 越界。");
        return false;
    }

    // 字段表紧跟 header，子字段也紧随其后（每条 kFieldDescSize 字节）。
    // 字符串池位置由 header 的 stringPoolOffset 显式给出，避免
    // “顶层字段数 × 定长”无法计入子字段的歧义。
    pos = payloadOffset;

    if (stringPoolOffset == 0 || stringPoolOffset > quint32(data.size())) {
        m_lastError = QStringLiteral("stringPoolOffset 越界。");
        return false;
    }

    // 第一遍：先解析字符串池（由 header 指定的位置开始）。
    StringPool strings;
    qint64 sp = stringPoolOffset;
    const quint32 stringCount = readLE<quint32>(data, sp);
    for (quint32 i = 0; i < stringCount; ++i) {
        const quint16 len = readLE<quint16>(data, sp);
        if (sp + len > data.size()) {
            m_lastError = QStringLiteral("字符串池越界。");
            return false;
        }
        QByteArray utf8(len, '\0');
        for (quint16 b = 0; b < len; ++b) {
            utf8[b] = data.at(int(sp++));
        }
        strings.mutableEntries().append(utf8);
    }

    // 第二遍：解析字段表。
    pos = payloadOffset;
    for (quint32 i = 0; i < fieldCount; ++i) {
        m_protocol.fields.append(readFieldDesc(data, pos, strings));
    }

    m_protocol.sourcePath = filePath;
    m_loaded = true;
    return true;
}

bool BinProtocolLoader::save(const QString &filePath) const
{
    // 序列化顺序：头 → 字段表（含子字段）→ 字符串池。
    // 注意：字符串序号需在写字段表时确定，所以先把所有字符串登记到池里，
    // 再把池写入。但字段表里记录的是序号，写表时即登记，可一次性完成：
    //   1) 写表时同步构建池（写入字段使用序号）；
    //   2) 由于序号在写表过程中递增，表与池的顺序必须一致，故先在内存里
    //      模拟一遍登记（写表 + 池构建），再拼最终字节。
    StringPool pool;
    QByteArray fieldTable;
    for (const FieldDef &f : m_protocol.fields) {
        writeFieldDesc(fieldTable, f, pool);
    }

    QByteArray stringPool;
    writeLE<quint32>(stringPool, quint32(pool.entries().size()));
    for (const QByteArray &s : pool.entries()) {
        writeLE<quint16>(stringPool, quint16(s.size()));
        stringPool.append(s);
    }

    // 头。
    QByteArray header;
    const QByteArray magic = m_protocol.magic.isEmpty()
        ? QByteArray("APROTO01", 8)
        : m_protocol.magic.toLatin1();
    // 保证 magic 恰好 8 字节。
    QByteArray magicFixed(8, '\0');
    for (int i = 0; i < 8 && i < magic.size(); ++i) {
        magicFixed[i] = magic.at(i);
    }
    header.append(magicFixed);
    writeLE<quint16>(header, m_protocol.version);
    writeLE<quint16>(header, m_protocol.flags);
    writeLE<quint32>(header, quint32(m_protocol.fields.size()));
    const quint32 payloadOffset = kBinProtocolHeaderSize;
    writeLE<quint32>(header, payloadOffset);
    // 字符串池起始偏移 = header + 字段表（含子字段）。
    const quint32 stringPoolOffset =
        kBinProtocolHeaderSize + quint32(fieldTable.size());
    writeLE<quint32>(header, stringPoolOffset);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(header);
    file.write(fieldTable);
    file.write(stringPool);
    file.close();
    return true;
}

void BinProtocolLoader::setProtocol(const BinProtocol &proto)
{
    m_protocol = proto;
    m_loaded = true;
    m_lastError.clear();
}

const BinProtocol &BinProtocolLoader::protocol() const
{
    return m_protocol;
}

bool BinProtocolLoader::isLoaded() const
{
    return m_loaded;
}

QString BinProtocolLoader::lastError() const
{
    return m_lastError;
}

BinProtocol BinProtocolLoader::makeSample()
{
    // 覆盖各字段类型的示例协议：一个设备状态帧。
    BinProtocol p;
    p.magic = QStringLiteral("APROTO01");
    p.version = 1;
    p.flags = 0;

    FieldDef frameId;
    frameId.name = QStringLiteral("frameId");
    frameId.type = FieldType::UInt16;
    frameId.byteOffset = 0;
    frameId.byteLength = 2;

    FieldDef deviceId;
    deviceId.name = QStringLiteral("deviceId");
    deviceId.type = FieldType::UInt32;
    deviceId.byteOffset = 2;
    deviceId.byteLength = 4;

    FieldDef temperature;
    temperature.name = QStringLiteral("temperature");
    temperature.type = FieldType::Float32;
    temperature.byteOffset = 6;
    temperature.byteLength = 4;
    temperature.comment = QStringLiteral("温度，单位 0.1℃");

    FieldDef statusFlags;
    statusFlags.name = QStringLiteral("statusFlags");
    statusFlags.type = FieldType::BitField;
    statusFlags.byteOffset = 10;
    statusFlags.byteLength = 1;
    statusFlags.bitOffset = 0;
    statusFlags.bitLength = 4;
    statusFlags.comment = QStringLiteral("低 4 位状态码");

    FieldDef mac;
    mac.name = QStringLiteral("mac");
    mac.type = FieldType::ByteArray;
    mac.byteOffset = 11;
    mac.byteLength = 6;
    mac.comment = QStringLiteral("MAC 地址");

    FieldDef deviceName;
    deviceName.name = QStringLiteral("deviceName");
    deviceName.type = FieldType::String;
    deviceName.byteOffset = 17;
    deviceName.byteLength = 16;
    deviceName.comment = QStringLiteral("设备名（UTF-8，定长 16 字节）");

    // 嵌套结构示例：2 维坐标 (x:int32, y:int32)。
    FieldDef coord;
    coord.name = QStringLiteral("coord");
    coord.type = FieldType::Struct;
    coord.byteOffset = 33;
    coord.byteLength = 8;
    FieldDef coordX;
    coordX.name = QStringLiteral("x");
    coordX.type = FieldType::Int32;
    coordX.byteOffset = 0;
    coordX.byteLength = 4;
    FieldDef coordY;
    coordY.name = QStringLiteral("y");
    coordY.type = FieldType::Int32;
    coordY.byteOffset = 4;
    coordY.byteLength = 4;
    coord.children.append(coordX);
    coord.children.append(coordY);

    p.fields.append(frameId);
    p.fields.append(deviceId);
    p.fields.append(temperature);
    p.fields.append(statusFlags);
    p.fields.append(mac);
    p.fields.append(deviceName);
    p.fields.append(coord);

    return p;
}
