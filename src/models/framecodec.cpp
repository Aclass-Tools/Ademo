#include "framecodec.h"

#include <QBitArray>
#include <QDebug>
#include <QRegularExpression>
#include <QtEndian>

namespace {

// 把整数按小端写入 buf 的指定位置，长度 1/2/4/8。
void writeLittleEndianAt(QByteArray &buf, int offset, quint64 value, int byteCount)
{
    for (int i = 0; i < byteCount; ++i) {
        const int idx = offset + i;
        if (idx < buf.size()) {
            buf[idx] = char((value >> (8 * i)) & 0xFF);
        }
    }
}

quint64 readLittleEndian(const QByteArray &buf, int byteCount, int start)
{
    quint64 v = 0;
    for (int i = 0; i < byteCount; ++i) {
        const int idx = start + i;
        const quint8 byte = (idx >= 0 && idx < buf.size()) ? quint8(buf.at(idx)) : 0;
        v |= quint64(byte) << (8 * i);
    }
    return v;
}

// 编码一个非结构字段，写入 out。
void encodeScalar(QByteArray &out, const FieldDef &f, const QVariant &value)
{
    const int off = int(f.byteOffset);
    const int len = int(f.byteLength);
    if (off < 0 || len <= 0) {
        return;
    }
    // 先确保缓冲区足够长。
    while (out.size() < off + len) {
        out.append('\0');
    }

    switch (f.type) {
    case FieldType::UInt8:
    case FieldType::Int8:
        writeLittleEndianAt(out, off, quint64(value.toULongLong() & 0xFF), 1);
        break;
    case FieldType::UInt16:
    case FieldType::Int16:
        writeLittleEndianAt(out, off, quint64(value.toULongLong() & 0xFFFF), 2);
        break;
    case FieldType::UInt32:
    case FieldType::Int32:
        writeLittleEndianAt(out, off, quint64(value.toULongLong() & 0xFFFFFFFFULL), 4);
        break;
    case FieldType::Float32: {
        const float fv = value.toFloat();
        quint32 bits = 0;
        memcpy(&bits, &fv, 4);
        bits = qToLittleEndian(bits);
        char *p = reinterpret_cast<char *>(&bits);
        for (int i = 0; i < 4 && i < len; ++i) {
            out[off + i] = p[i];
        }
        break;
    }
    case FieldType::Float64: {
        const double dv = value.toDouble();
        quint64 bits = 0;
        memcpy(&bits, &dv, 8);
        bits = qToLittleEndian(bits);
        char *p = reinterpret_cast<char *>(&bits);
        for (int i = 0; i < 8 && i < len; ++i) {
            out[off + i] = p[i];
        }
        break;
    }
    case FieldType::BitField: {
        // 位字段：把 value 的低 bitLength 位写入 byteOffset 起的位区间，
        // 区间内低位对齐（bitOffset 为低位起点），保留其它位不变。
        const int totalBits = int(f.bitOffset) + int(f.bitLength);
        const int bytesNeeded = (totalBits + 7) / 8;
        quint64 existing = readLittleEndian(out, bytesNeeded, off);
        const quint64 mask = (f.bitLength >= 64)
            ? ~quint64(0)
            : ((quint64(1) << f.bitLength) - 1);
        const quint64 v = quint64(value.toULongLong()) & mask;
        existing &= ~(mask << f.bitOffset);
        existing |= (v << f.bitOffset);
        writeLittleEndianAt(out, off, existing, bytesNeeded);
        break;
    }
    case FieldType::ByteArray: {
        // 字节数组：从 hex 或原样取前 len 字节。
        const QByteArray src = FrameCodec::fromHexDisplay(value.toString());
        const QByteArray use = src.isEmpty() ? value.toByteArray() : src;
        for (int i = 0; i < len; ++i) {
            out[off + i] = (i < use.size()) ? use.at(i) : char(0);
        }
        break;
    }
    case FieldType::String: {
        // 定长 UTF-8 字符串，不足补 0，超长截断。
        const QByteArray utf8 = value.toString().toUtf8();
        for (int i = 0; i < len; ++i) {
            out[off + i] = (i < utf8.size()) ? utf8.at(i) : char(0);
        }
        break;
    }
    case FieldType::Struct:
        // 由 encode() 主流程递归处理，这里不应到达。
        break;
    }
}

// 解码一个非结构字段。
QVariant decodeScalar(const QByteArray &frame, const FieldDef &f)
{
    const int off = int(f.byteOffset);
    const int len = int(f.byteLength);
    if (off < 0 || len <= 0) {
        return {};
    }

    switch (f.type) {
    case FieldType::UInt8:
        return QVariant(quint32(readLittleEndian(frame, 1, off) & 0xFF));
    case FieldType::Int8:
        return QVariant(qint32(qint8(readLittleEndian(frame, 1, off) & 0xFF)));
    case FieldType::UInt16:
        return QVariant(quint32(readLittleEndian(frame, 2, off) & 0xFFFF));
    case FieldType::Int16:
        return QVariant(qint32(qint16(readLittleEndian(frame, 2, off) & 0xFFFF)));
    case FieldType::UInt32:
        return QVariant(quint64(readLittleEndian(frame, 4, off) & 0xFFFFFFFFULL));
    case FieldType::Int32:
        return QVariant(qint64(qint32(readLittleEndian(frame, 4, off) & 0xFFFFFFFFu)));
    case FieldType::Float32: {
        quint32 bits = qFromLittleEndian(quint32(readLittleEndian(frame, 4, off) & 0xFFFFFFFFu));
        float fv = 0.0f;
        memcpy(&fv, &bits, 4);
        return QVariant(double(fv));
    }
    case FieldType::Float64: {
        quint64 bits = qFromLittleEndian(readLittleEndian(frame, 8, off));
        double dv = 0.0;
        memcpy(&dv, &bits, 8);
        return QVariant(dv);
    }
    case FieldType::BitField: {
        const int totalBits = int(f.bitOffset) + int(f.bitLength);
        const int bytesNeeded = (totalBits + 7) / 8;
        const quint64 raw = readLittleEndian(frame, bytesNeeded, off);
        const quint64 mask = (f.bitLength >= 64)
            ? ~quint64(0)
            : ((quint64(1) << f.bitLength) - 1);
        return QVariant(quint64((raw >> f.bitOffset) & mask));
    }
    case FieldType::ByteArray: {
        QByteArray out(len, '\0');
        for (int i = 0; i < len; ++i) {
            const int idx = off + i;
            out[i] = (idx < frame.size()) ? frame.at(idx) : char(0);
        }
        return QVariant(FrameCodec::toHexDisplay(out));
    }
    case FieldType::String: {
        QByteArray utf8(len, '\0');
        for (int i = 0; i < len; ++i) {
            const int idx = off + i;
            utf8[i] = (idx < frame.size()) ? frame.at(idx) : char(0);
        }
        // 截断到第一个 \0。
        const int nul = utf8.indexOf('\0');
        if (nul >= 0) {
            utf8 = utf8.left(nul);
        }
        return QVariant(QString::fromUtf8(utf8));
    }
    case FieldType::Struct:
        return {};
    }
    return {};
}

} // namespace

QByteArray FrameCodec::encode(const BinProtocol &proto, const QMap<QString, QVariant> &values)
{
    // 初始缓冲区按 frameSize 预分配并填 0。
    const quint32 size = proto.frameSize();
    QByteArray out(int(size), '\0');

    for (const FieldDef &f : proto.fields) {
        if (f.type == FieldType::Struct) {
            // 递归编码子字段（本轮一层）。
            QByteArray sub(int(f.byteLength), '\0');
            const int base = int(f.byteOffset);
            // 先把 sub 初始化为主帧该区间已有内容（避免互相覆盖丢失）。
            for (int i = 0; i < int(f.byteLength) && base + i < out.size(); ++i) {
                sub[i] = out.at(base + i);
            }
            for (const FieldDef &c : f.children) {
                FieldDef shifted = c;
                shifted.byteOffset = base + c.byteOffset;
                // 子字段在 sub 内偏移，再写入 out。
                encodeScalar(out, shifted, values.value(QStringLiteral("%1.%2").arg(f.name, c.name)));
            }
        } else {
            encodeScalar(out, f, values.value(f.name));
        }
    }
    return out;
}

QMap<QString, QVariant> FrameCodec::decode(const BinProtocol &proto, const QByteArray &frame)
{
    QMap<QString, QVariant> result;
    for (const FieldDef &f : proto.fields) {
        if (f.type == FieldType::Struct) {
            for (const FieldDef &c : f.children) {
                FieldDef shifted = c;
                shifted.byteOffset = f.byteOffset + c.byteOffset;
                const QString key = QStringLiteral("%1.%2").arg(f.name, c.name);
                result.insert(key, decodeScalar(frame, shifted));
            }
            // 父字段值也放一份原始 hex，便于展示。
            const int off = int(f.byteOffset);
            const int len = int(f.byteLength);
            QByteArray sub(len, '\0');
            for (int i = 0; i < len; ++i) {
                const int idx = off + i;
                sub[i] = (idx < frame.size()) ? frame.at(idx) : char(0);
            }
            result.insert(f.name, QVariant(toHexDisplay(sub)));
        } else {
            result.insert(f.name, decodeScalar(frame, f));
        }
    }
    return result;
}

QString FrameCodec::valueToString(const FieldDef &field, const QVariant &value)
{
    switch (field.type) {
    case FieldType::Float32:
    case FieldType::Float64:
        return QString::number(value.toDouble());
    case FieldType::ByteArray:
        return value.toString();
    case FieldType::String:
        return value.toString();
    default:
        return value.toString();
    }
}

QString FrameCodec::toHexDisplay(const QByteArray &data, const QString &sep)
{
    QString out;
    out.reserve(data.size() * 3);
    for (int i = 0; i < data.size(); ++i) {
        if (i > 0) {
            out += sep;
        }
        out += QString::asprintf("%02X", quint8(data.at(i)));
    }
    return out;
}

QByteArray FrameCodec::fromHexDisplay(const QString &hex)
{
    // 兼容 "AA BB" / "AABB" / "aa bb" 等形式。
    QString compact = hex;
    compact.remove(QRegularExpression(QStringLiteral("[\\s,;]+")));
    if (compact.isEmpty()) {
        return {};
    }
    // 校验：偶数个十六进制字符。
    static const QRegularExpression re(QStringLiteral("^[0-9A-Fa-f]+$"));
    if (compact.length() % 2 != 0 || !re.match(compact).hasMatch()) {
        return {};
    }
    return QByteArray::fromHex(compact.toUtf8());
}
