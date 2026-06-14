// 临时端到端验证：确认 BinProtocolLoader / FrameCodec 与 Python 生成的 .bin 一致。
// 验证项：
//  1. 加载 Python 生成的 sample_protocol.bin 成功，字段数 = 7。
//  2. encode 后 decode 往返，关键字段值一致。
//  3. save → 重新 load 往返一致。
// 成功输出 "ALL OK"，失败输出原因并返回非 0。

#include "models/binprotocolformat.h"
#include "models/binprotocolloader.h"
#include "models/framecodec.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QVariant>
#include <QString>
#include <QStringList>

#include <cstdio>

static int failures = 0;
static void logp(const char *tag, const QString &msg)
{
    const QByteArray u8 = msg.toUtf8();
    std::printf("%s: %s\n", tag, u8.constData());
    std::fflush(stdout);
}
#define CHECK(cond, msg) do { \
    if (!(cond)) { logp("FAIL", msg); ++failures; } \
    else { logp("ok", msg); } } while (0)
#define LOG(msg) logp("info", msg)

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // 从可执行目录往上找项目根（data/protocols 所在）。
    QDir dir(app.applicationDirPath());
    QString binPath;
    for (int i = 0; i < 6; ++i) {
        const QString candidate = dir.absoluteFilePath(QStringLiteral("data/protocols/sample_protocol.bin"));
        if (QFile::exists(candidate)) {
            binPath = candidate;
            break;
        }
        dir.cdUp();
    }
    if (binPath.isEmpty()) {
        binPath = app.applicationDirPath() + QStringLiteral("/data/protocols/sample_protocol.bin");
    }

    // 1. 加载示例 .bin。
    BinProtocolLoader loader;
    CHECK(QFile::exists(binPath), QStringLiteral("文件存在: %1").arg(binPath));
    const bool loaded = loader.load(binPath);
    CHECK(loaded, QStringLiteral("加载成功"));
    if (!loaded) {
        LOG(QStringLiteral("load error: %1").arg(loader.lastError()));
        return 1;
    }
    const BinProtocol &proto = loader.protocol();
    CHECK(proto.fields.size() == 7, QStringLiteral("顶层字段数=7, 实际=%1").arg(proto.fields.size()));
    CHECK(proto.frameSize() == 41, QStringLiteral("帧大小=41, 实际=%1").arg(proto.frameSize()));

    // 校验字段类型与偏移。
    CHECK(proto.fields.at(0).name == QStringLiteral("frameId"), "field0 name");
    CHECK(proto.fields.at(0).type == FieldType::UInt16, "field0 type");
    CHECK(proto.fields.at(6).type == FieldType::Struct, "coord is Struct");
    CHECK(proto.fields.at(6).children.size() == 2, "coord has 2 children");

    // 2. 组包往返。
    QMap<QString, QVariant> values;
    values.insert(QStringLiteral("frameId"), QVariant(quint64(0x1234)));
    values.insert(QStringLiteral("deviceId"), QVariant(quint64(0xAABBCCDD)));
    values.insert(QStringLiteral("temperature"), QVariant(double(256.5)));
    values.insert(QStringLiteral("statusFlags"), QVariant(quint64(0x9))); // 4 位
    values.insert(QStringLiteral("mac"), QVariant(QStringLiteral("00 11 22 33 44 55")));
    values.insert(QStringLiteral("deviceName"), QVariant(QStringLiteral("hello")));
    values.insert(QStringLiteral("coord.x"), QVariant(qint64(-100)));
    values.insert(QStringLiteral("coord.y"), QVariant(qint64(200)));

    const QByteArray frame = FrameCodec::encode(proto, values);
    CHECK(frame.size() == 41, QStringLiteral("encode 长度=41, 实际=%1").arg(frame.size()));
    LOG(QStringLiteral("encoded hex: %1").arg(FrameCodec::toHexDisplay(frame)));

    const QMap<QString, QVariant> decoded = FrameCodec::decode(proto, frame);
    CHECK(decoded.value(QStringLiteral("frameId")).toULongLong() == 0x1234, "decode frameId");
    CHECK(decoded.value(QStringLiteral("deviceId")).toULongLong() == 0xAABBCCDD, "decode deviceId");
    CHECK(qAbs(decoded.value(QStringLiteral("temperature")).toDouble() - 256.5) < 0.01, "decode temperature");
    CHECK(decoded.value(QStringLiteral("statusFlags")).toULongLong() == 0x9, "decode statusFlags");
    CHECK(decoded.value(QStringLiteral("deviceName")).toString() == QStringLiteral("hello"), "decode deviceName");
    CHECK(decoded.value(QStringLiteral("coord.x")).toLongLong() == -100, "decode coord.x");
    CHECK(decoded.value(QStringLiteral("coord.y")).toLongLong() == 200, "decode coord.y");

    // 3. save → load 往返。
    const QString tmpPath = app.applicationDirPath() + QStringLiteral("/roundtrip_tmp.bin");
    CHECK(loader.save(tmpPath), "save 成功");
    BinProtocolLoader loader2;
    const bool rl = loader2.load(tmpPath);
    CHECK(rl, "reload 成功");
    if (rl) {
        const BinProtocol &p2 = loader2.protocol();
        CHECK(p2.fields.size() == proto.fields.size(), "roundtrip 字段数一致");
        bool namesMatch = true;
        for (int i = 0; i < proto.fields.size() && i < p2.fields.size(); ++i) {
            if (proto.fields[i].name != p2.fields[i].name
                || proto.fields[i].type != p2.fields[i].type
                || proto.fields[i].byteOffset != p2.fields[i].byteOffset
                || proto.fields[i].byteLength != p2.fields[i].byteLength) {
                namesMatch = false;
            }
        }
        CHECK(namesMatch, "roundtrip 字段内容一致");
    }
    QFile::remove(tmpPath);

    LOG(failures == 0 ? QStringLiteral("ALL OK") : QStringLiteral("FAILED %1 checks").arg(failures));
    return failures == 0 ? 0 : 1;
}
