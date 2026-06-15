# 协议编辑后端（Flask + SQLite）

协议编辑功能已从 Qt 桌面端迁移到 Web。本目录是一个独立的 Python 后端：

- **数据库**：SQLite（单文件 `protocols.db`，零配置）。
- **Web 后端**：Flask，提供 REST API 与静态前端。
- **bin 生成**：按 Qt 端 `BinProtocolLoader` 的字节布局实时生成 `.bin`，供 Qt 下载加载。

> Qt 端不再内嵌编辑界面，改用系统浏览器打开本后端的网页进行编辑；编辑保存后回到 Qt 协议编辑页下载最新 `.bin`。

## 架构

```
┌─────────────────┐   系统浏览器    ┌──────────────────┐  sqlite3   ┌──────────┐
│ Flask 后端       │ <───────────> │ 原生 HTML + Vue   │ <────────> │ SQLite    │
│ :5000           │   HTTP         │ CDN 前端(静态)     │            │ protocols │
│  - REST API     │                │ 字段表 CRUD        │            │  .db      │
│  - 生成 bin      │                │ Struct 子字段编辑  │            └──────────┘
│  - 托管前端      │                │ 生成/下载 bin      │
└────┬────────────┘                └──────────────────┘
     │ HTTP GET /api/protocols/{id}/bin
     ▼
┌─────────────────┐
│ Qt (现有 GCC)    │  点击「打开 Web 编辑器」→ 系统浏览器
│ ApiClient        │  点击「下载并加载 .bin」→ 下载到 data/protocols/
│ BinProtocolLoader│  → 加载本地 .bin
└─────────────────┘
```

**不安装任何 Qt 组件、不改编译器、不修改 bin 字节格式。**

## 目录结构

```
backend/
├── app.py              # Flask 主应用 + 全部路由
├── bin_writer.py       # bin 生成器（移植自 scripts/generate_sample_bin.py）
├── models.py           # SQLite 数据访问层（protocol / field）
├── db.py               # 数据库连接与建表
├── init_db.py          # 首次初始化 + 导入示例协议
├── requirements.txt    # Flask
├── run_backend.bat     # 命令行启动脚本（首次自动装 Flask）
├── run_backend.pyw     # 无控制台启动入口（快捷方式用）
├── protocols.db        # SQLite 数据库（首次运行自动生成）
└── static/             # 前端静态文件（被 Flask 托管）
    ├── index.html
    ├── app.js          # Vue 3 (CDN) 应用
    └── style.css       # 工业蓝风格
```

## 启动

### 方式一：批处理（推荐）

双击 `backend\run_backend.bat`。

- 首次运行会自动 `pip install flask` 并建库、导入示例协议。
- 之后每次启动直接运行 `python app.py`。

### 方式二：命令行

```bat
cd backend
python -m pip install -r requirements.txt   :: 仅首次
python app.py
```

### 方式三：无控制台（做成快捷方式）

把 `run_backend.pyw` 建成桌面快捷方式，双击即后台启动（用 `pythonw` 运行，无黑窗）。

启动后访问 <http://127.0.0.1:5000>。

## 数据模型

`protocol` 表（协议元信息）：

| 列          | 类型    | 说明                         |
|-------------|---------|------------------------------|
| id          | INTEGER | 主键                         |
| name        | TEXT    | 协议名（也作为 bin 文件名）  |
| version     | INTEGER | 格式版本（默认 1）           |
| flags       | INTEGER | 预留（默认 0）               |
| updated_at  | TEXT    | 最后更新时间                 |

`field` 表（字段，含一层嵌套子字段）：

| 列              | 类型    | 说明                                            |
|-----------------|---------|-------------------------------------------------|
| id              | INTEGER | 主键                                            |
| protocol_id     | INTEGER | 所属协议（外键，级联删除）                      |
| parent_field_id | INTEGER | NULL=顶层；非空=某 Struct 字段的子字段（仅一层）|
| seq             | INTEGER | 同一父下的顺序                                  |
| name            | TEXT    | 字段名                                          |
| type            | TEXT    | FieldType 名：UInt8..Struct（共 12 种）         |
| byte_offset     | INTEGER | 在帧中的字节偏移                                |
| byte_length     | INTEGER | 字节长度                                        |
| bit_offset      | INTEGER | 位字段：起始位                                  |
| bit_length      | INTEGER | 位字段：位数                                    |
| comment         | TEXT    | 备注                                            |

## REST API

| 方法   | 路径                              | 说明                                   |
|--------|-----------------------------------|----------------------------------------|
| GET    | `/`                               | 前端入口（重定向到 static/index.html） |
| GET    | `/api/health`                     | 健康检查                               |
| GET    | `/api/protocols`                  | 列出所有协议                           |
| POST   | `/api/protocols`                  | 新建协议（body: `{name,version,flags}`)|
| GET    | `/api/protocols/<id>`             | 获取协议（含 fields）                  |
| PUT    | `/api/protocols/<id>`             | 更新协议元信息                         |
| DELETE | `/api/protocols/<id>`             | 删除协议                               |
| GET    | `/api/protocols/<id>/fields`      | 字段列表（含子字段）                   |
| PUT    | `/api/protocols/<id>/fields`      | 整体替换字段表                         |
| GET    | `/api/protocols/<id>/bin`         | **生成并下载 .bin**（Qt 用此接口）     |

### bin 接口示例

```bat
curl http://127.0.0.1:5000/api/protocols/1/bin -o my_protocol.bin
```

返回 `application/octet-stream`，`Content-Disposition` 带协议名（UTF-8 编码，兼容中文）。

## bin 字节布局（与 Qt `BinProtocolLoader` 严格一致）

全小端：

1. **头 24 字节**：`magic(8)="APROTO01" + version(2) + flags(2) + fieldCount(4) + payloadOffset(4) + stringPoolOffset(4)`
2. **字段描述表**：顶层字段后紧跟各 Struct 的子字段，每条 24 字节
   `nameIdx(4) type(1) byteOffset(4) byteLength(2) bitOffset(1) bitLength(1) childCount(2) reserved(1) commentIdx(4) pad(4)`
3. **字符串池**：`count(4) + N × (len(2) + utf8 bytes)`（内容相同的字符串复用同一序号）

> 该布局直接移植自 `scripts/generate_sample_bin.py`，已通过字节级比对验证：后端生成的 bin 与 Qt 已有 `data/protocols/sample_protocol.bin`（431 字节）**md5 完全一致**。

## 与 Qt 的集成

Qt 端（`ProtocolEditorPage`）改为"桥接页"：

- **打开 Web 编辑器**：`QDesktopServices::openUrl("http://127.0.0.1:5000")` 调系统浏览器。
- **下载并加载 .bin**：`ApiClient::getProtocolBin(id, 路径)` 下载到 `data/protocols/<name>.bin`，再用现有 `BinProtocolLoader::load()` 加载。

Qt 端不依赖 WebEngine、WebChannel，纯 GCC 工程不变。

## 开发说明

- 前端用 Vue 3 全局构建（CDN），无构建步骤，改完 `static/` 刷新浏览器即可。
- 改数据库结构后删 `protocols.db` 重启即可重建（首次会自动导入示例协议）。
- 想换端口：改 `app.py` 末尾的 `app.run(port=...)`，同时改 Qt 端 `ApiClient::setProtocolBackendUrl()`。
