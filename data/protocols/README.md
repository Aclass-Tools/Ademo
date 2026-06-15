# data/protocols — 协议描述文件

存放自定义 `.bin` 协议描述文件，供"协议调试页 / 协议编辑页"加载使用。

## 文件格式

自定义紧凑二进制布局，**全部小端**。详细定义见 `src/models/binprotocolformat.h`，
序列化逻辑见 `src/models/binprotocolloader.cpp`。

```
┌──────────────────────────────────────────────────────────┐
│ Header (24 字节)                                          │
│   magic[8]          = "APROTO01"                          │
│   version (u16)     = 1                                   │
│   flags   (u16)     = 0                                   │
│   fieldCount (u32)  = 顶层字段数（不含 Struct 子字段）     │
│   payloadOffset (u32) = 字段表起始（固定 = 24）            │
│   stringPoolOffset (u32) = 字符串池起始                   │
├──────────────────────────────────────────────────────────┤
│ 字段表：每条 24 字节，顶层字段后紧跟各 Struct 的子字段      │
│   nameIdx(u32) type(u8) byteOffset(u32) byteLength(u16)  │
│   bitOffset(u8) bitLength(u8) childCount(u16)            │
│   reserved(u8) commentIdx(u32) pad(4)                     │
├──────────────────────────────────────────────────────────┤
│ 字符串池：count(u32) + N × (len(u16) + utf8 bytes)        │
└──────────────────────────────────────────────────────────┘
```

## 支持的字段类型（FieldType）

整数 u8/u16/u32、i8/i16/i32、float32/64、ByteArray、BitField（含跨字节）、
String（定长 UTF-8）、Struct（本轮支持一层嵌套）。

## 生成示例

```powershell
python scripts\generate_sample_bin.py
# → 生成 data/protocols/sample_protocol.bin（431 字节，覆盖全部 7 类型）
```

## 一致性测试

`tests/tmp/` 下有端到端测试程序，验证 Python 写入与 Qt 读取往返一致。
详见 `tests/tmp/README.md`。

## 从协议编辑后端下载（推荐流程）

协议编辑已迁移到 Web。`.bin` 除了用上面的 Python 脚本生成，也可由
`backend/`（Flask + SQLite）按数据库内容实时生成：

```bat
:: 启动后端（首次会自动建库 + 导入示例）
backend\run_backend.bat

:: 下载某协议的 bin 到本目录
curl http://127.0.0.1:5000/api/protocols/1/bin -o sample_protocol.bin
```

- 后端 bin 生成逻辑见 `backend/bin_writer.py`，字节布局与本文件描述完全一致。
- Qt 协议编辑页可直接「下载并加载 .bin」，自动落到本目录后再用 `BinProtocolLoader` 加载。
- 详见 `backend/README.md`。

