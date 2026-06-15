# AclassTool

A Qt6 Widgets application template organized with a practical, scalable structure inspired by real-world Qt projects.

## Directory Layout

```text
AclassTool/
├─ CMakeLists.txt
├─ cmake/                # Reusable CMake modules
│  ├─ AclassPlatform.cmake
│  ├─ AclassProjectOptions.cmake
│  └─ README.md
├─ src/                  # C++ source and headers
│  ├─ CMakeLists.txt
│  ├─ main.cpp
│  ├─ models/            # 数据模型层（.bin 协议加载/编解码、字段表模型）
│  ├─ network/           # 网络层（HTTP/WS 客户端、串口管理）
│  ├─ services/          # 服务层（统一连接管理 + 传输封装协议 A0/A3/Modbus）
│  └─ ui/                # 主窗口、主题、各功能页面（pages/）
├─ ui/                   # Qt Designer .ui files
│  └─ mainwindow.ui
├─ ui/                   # Qt Designer .ui files
│  └─ mainwindow.ui
├─ resources/            # Static assets and Qt resource collection
│  ├─ app.qrc
│  └─ icons/
├─ dist/                 # Packaging entry points (windeployqt, installers)
│  ├─ CMakeLists.txt
│  ├─ debug/
│  ├─ release/
│  ├─ installer/
│  └─ README.md
├─ scripts/              # Bash/PowerShell helper scripts
│  ├─ cmake-tools.sh
│  ├─ cmake-tools.ps1
│  └─ README.md
├─ aclassenv.sh          # Bash root script entry
├─ aclassenv.ps1         # PowerShell root script entry
├─ tests/                # Unit/integration tests
│  ├─ CMakeLists.txt
│  └─ README.md
├─ build/                # Out-of-source build directory (generated)
└─ .vscode/
```

## Toolchain (Qt 6.8.3 + MinGW 13.1 + Ninja)

本项目使用 Qt 官方安装器提供的工具链，默认安装路径如下（与 `scripts/cmake-tools.ps1` 的默认值一致）：

| 组件 | 路径 |
|---|---|
| Qt 前缀 | `C:/Qt/6.8.3/mingw_64` |
| MinGW 13.1 (gcc/g++) | `C:/Qt/Tools/mingw1310_64/bin` |
| Ninja | `C:/Qt/Tools/Ninja/ninja.exe` |
| CMake | `C:/Qt/Tools/CMake_64/bin/cmake.exe` |
| windeployqt | `C:/Qt/6.8.3/mingw_64/bin/windeployqt.exe` |
| NSIS（打包安装包可选） | `C:/Program Files (x86)/NSIS/makensis.exe` |

> 注意：系统 PATH 中通常**没有** cmake/qmake。若直接用脚本，需要先设置 `ACLASS_CMAKE_EXE` 环境变量指向 `C:/Qt/Tools/CMake_64/bin/cmake.exe`，否则脚本会找不到 cmake。

Qt 安装时需勾选以下组件：**Qt 6.8.3 / MinGW 64-bit**、**MinGW 13.1.0 64-bit**、**Ninja**、**CMake**。本项目依赖的 Qt 模块：`Widgets Network WebSockets QuickWidgets SerialPort SerialBus`（SerialBus 用于 Modbus TCP/RTU）。

## Manual Build (Recommended)

手工方式最直观，能精确控制各工具路径。以 Debug 为例（cmd 或 PowerShell 均可）：

```powershell
# 1) 把 MinGW 加入 PATH（编译器/运行时 DLL）
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;%PATH%

# 2) 配置（首次或 CMakeLists 变更后执行）
"C:\Qt\Tools\CMake_64\bin\cmake.exe" -S . -B build/Debug -G Ninja ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64 ^
    -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe ^
    -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe ^
    -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe ^
    -DWINDEPLOYQT_EXECUTABLE=C:/Qt/6.8.3/mingw_64/bin/windeployqt.exe ^
    -DNSIS_MAKENSIS_EXECUTABLE="C:/Program Files (x86)/NSIS/makensis.exe"

# 3) 构建
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/Debug

# 4) 部署（拷贝 exe + 自动补齐 Qt DLL，含 Qt6SerialPort.dll）
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/Debug --target dist_win

# 5) 运行
dist\debug\AclassTool.exe
```

Release 构建把上述 `Debug` 全部换成 `Release`、`build/Debug` 换成 `build/Release`、`dist/debug` 换成 `dist/release` 即可。

## Build Outputs

| 目录 | 内容 | 能否直接运行 |
|---|---|---|
| `build/Debug/` | 仅编译产物 `AclassTool.exe` | ❌ 缺 Qt DLL |
| `dist/debug/` | exe + 全部依赖 DLL（`dist_win` 产出） | ✅ 可直接双击 |
| `build/Release/` + `dist/release/` | 同上，Release 版 | ✅ |
| `dist/installer/` | NSIS 安装包（`dist_installer` 产出） | ✅ |

> `dist_win` 用 windeployqt 自动识别依赖并补齐 DLL（含 `Qt6SerialPort.dll`），是日常运行/分发的主入口。

## Scripted Build Workflow (PowerShell)

> 前置：设置 cmake 路径，否则脚本默认的 `cmake` 找不到。
> ```powershell
> $env:ACLASS_CMAKE_EXE = "C:\Qt\Tools\CMake_64\bin\cmake.exe"
> ```

Load functions:

```powershell
. .\aclassenv.ps1
```

Then call:

```powershell
Abuild
Abuild Release
Ainstall
Adist
Apack
Aclean
```

Direct CLI mode:

```powershell
.\aclassenv.ps1 build
.\aclassenv.ps1 build release
.\aclassenv.ps1 install
.\aclassenv.ps1 dist
.\aclassenv.ps1 pack
.\aclassenv.ps1 clean
```

## Scripted Build Workflow (Bash)

> 前置：设置 cmake 路径，否则脚本默认的 `cmake` 找不到。
> ```bash
> export ACLASS_CMAKE_EXE="C:/Qt/Tools/CMake_64/bin/cmake.exe"
> ```

Load functions:

```bash
source ./aclassenv.sh
```

Then call:

```bash
Abuild
Abuild Release
Ainstall
Adist
Apack
Aclean
```

Direct CLI mode:

```bash
./aclassenv.sh build
./aclassenv.sh build release
./aclassenv.sh install
./aclassenv.sh dist
./aclassenv.sh pack
./aclassenv.sh clean
```

## Default Behavior

- `Abuild` default: `Debug`
- `Aclean` default: `all`
- `Ainstall` default: `Release`
- `Adist` default: `Release`
- `Apack` default: `Release`

## Qt AUTOUIC Note

- UI files are stored in `ui/`, while C++ sources are in `src/`.
- The project sets `AUTOUIC_SEARCH_PATHS` to `ui/` so includes like `#include "ui_mainwindow.h"` resolve correctly.

## Functional Modules

| 模块 | 路径 | 说明 |
|---|---|---|
| 模型层 | `src/models/` | 点位表、JSON 预览、**.bin 协议加载/编解码**（`binprotocolloader`、`framecodec`、`protocolfieldtablemodel`） |
| **服务层** | `src/services/` | **统一连接 + 传输封装协议**：`ConnectionManager`（串口/网口/Modbus 全局共享）、`ProtocolParser`（A0/A3 帧 + CRC16）、`ModbusService`（Qt SerialBus） |
| 网络层 | `src/network/` | HTTP/WS 客户端、串口管理（`serialportmanager`，被 services 层复用） |
| UI 页面 | `src/ui/pages/` | 设备连接、协议调试、指令操作、设备升级、终端调试、协议编辑（6 页） |
| 协议数据 | `data/protocols/` | 自定义 `.bin` 协议示例（格式规范见 `data/protocols/README.md`） |
| 协议生成 | `scripts/generate_sample_bin.py` | 生成覆盖全部字段类型的示例 `.bin` |
| 协议编辑后端 | `backend/` | Flask + SQLite + Vue CDN，Web 编辑协议并按数据库实时生成 `.bin`（详见下文） |
| 协议测试 | `tests/tmp/` | 协议层端到端自测（Python↔Qt 格式一致性） |

### 业务页面（已实现，6 页）

- **设备连接**：串口/网口切换 + 完整串口参数（端口/波特率/数据位/校验/停止位/流控），建立全局共享连接
- **协议调试**：加载 `.bin` → 字段表组包 → 按协议自带的传输类型（A0/A3/Modbus）封装发送 / 接收拆帧解析（两层协议协作）
- **指令操作**：按字段批量读/写（单行读/写、总召、总写），回包自动回填表格
- **设备升级**：选固件 → 分块用 A0 帧封装 → 经统一连接发送 → 进度/日志
- **终端调试**：订阅统一连接的裸字节流，Hex/ASCII 收发、时间戳、收发计数、定时发送
- **协议编辑**：Web 桥接页——打开系统浏览器编辑协议（可选 A0/A3/Modbus）、从后端下载 `.bin` 并加载

### `.bin` 协议格式

自定义紧凑二进制（小端），支持：整数 u/i 8/16/32、float32/64、ByteArray、BitField（跨字节）、String、Struct（一层嵌套）。
完整规范见 [`data/protocols/README.md`](data/protocols/README.md)，编解码实现见 `src/models/framecodec.cpp`。

每个协议还自带**传输封装类型**（A0 / A3 / Modbus），编码进文件头 `flags` 字段的低 2 位（0=A0, 1=A3, 2=Modbus），向后兼容（旧文件 flags=0 即 A0）。两层协议正交：
- **字段表层**（`FrameCodec`）：payload 内部字段如何排布。
- **传输封装层**（`ProtocolParser`）：payload 外面包什么帧头/地址/CRC（A0/A3）；Modbus 走 Qt SerialBus。

## 协议编辑后端（Flask + SQLite + Web）

协议编辑已从 Qt 桌面端迁移到 Web，Qt 端的「协议编辑」页改为**桥接页**（打开系统浏览器编辑 → 从后端下载 `.bin` → 用现有 `BinProtocolLoader` 加载）。**不安装 QtWebEngine、不改编译器、不改 bin 字节格式。**

架构与数据流：

```
┌──────────────┐  系统浏览器   ┌──────────────┐  sqlite3  ┌────────┐
│ Flask :5000  │ <──────────> │ Vue CDN 前端  │ <───────> │ SQLite │
│  REST + 托管  │              │ 字段表编辑     │           │ .db    │
│  生成 .bin    │              │ Struct 子字段  │           └────────┘
└────┬─────────┘              └──────────────┘
     │ HTTP GET /api/protocols/<id>/bin
     ▼
┌──────────────┐  点「打开 Web 编辑器」→ 系统浏览器
│ Qt (GCC 工程) │  点「下载并加载 .bin」→ 下载到 data/protocols/
│ BinProtocolLoader → 加载本地 .bin
└──────────────┘
```

详细文档见 [`backend/README.md`](backend/README.md)。

### 启动后端

```bat
:: 首次会自动 pip install flask + 建库 + 导入示例协议
backend\run_backend.bat
```

启动后浏览器访问 <http://127.0.0.1:5000> 编辑协议、保存、生成/下载 `.bin`。
（也可用 `backend\run_backend.pyw` 无控制台后台启动，适合做桌面快捷方式。）

### 编译 Qt 端（与上面「Manual Build」同一套命令）

协议编辑页的改动只是普通 C++ + .ui，不需要新增任何 Qt 模块。用前文的工具链 PATH 编译即可：

```powershell
# 1) 工具链 PATH（必须，否则 g++ 找不到运行时 DLL 会静默失败）
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;%PATH%

# 2) 构建（已配置过则只需这步）
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/Debug

# 3) 部署（生成可双击运行的 dist\debug\AclassTool.exe）
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/Debug --target dist_win

# 4) 运行
dist\debug\AclassTool.exe
```

> 端到端联调顺序：先 `run_backend.bat` 起后端 → 再运行 `AclassTool.exe` → 点「协议编辑」页 → 「打开 Web 编辑器」编辑保存 → 回 Qt 点「下载并加载 .bin」→ 协议调试页即可用新协议。

## Design Documents

中文设计文档（架构 / 数据库 / 后端 / 前端）：

- `develop1.md` — 整体设计思路
- `develop2.md` — 数据库设计
- `develop3.md` — 后端设计
- `develop4.md` — Qt 前端界面设计
