# services 层 — 统一连接管理 + 传输封装协议

借鉴 QtIndustrialClient 的分层思路，把"连接管理"和"传输封装协议"从各业务页面剥离，
集中到本层。所有页面通过 `ConnectionManager` 收发，不再各自持有 `SerialPortManager`。

## 设计目标

- **全局共享一个连接**：任意时刻只有一个活动 Transport（串口/网口/Modbus）。
  彻底解决旧架构里多个页面各自打开串口、配置不一致、端口争用的问题。
- **两层协议正交**：
  - 字段表层（`models/FrameCodec`）— payload 内部字段如何排布（u16 在偏移 0…）。
  - 传输封装层（`ProtocolParser`）— payload 外面包什么帧头/地址/CRC（A0/A3）。
  - Modbus 走 Qt SerialBus，不经过 ProtocolParser。
- **统一信号面**：页面只订阅 `frameReceived`（拆好的帧）/ `rawBytesReceived`（裸字节，终端用）/ `modbusReplies`（Modbus 寄存器值），不关心数据来自哪个 Transport。

## 文件

| 文件 | 职责 |
|------|------|
| `connectionmanager.h/.cpp` | 全局连接入口（MainWindow 持有单例）。`connectSerial`/`connectNetwork`/`connectModbus`/`disconnect`/`sendFrame`/`sendRaw`。信号：`frameReceived`、`rawBytesReceived`、`modbusReplies`、`connectionChanged`、`log`、`errorOccurred`。 |
| `serialservice.h/.cpp` | 串口传输，复用 `network/SerialPortManager`，叠加 `ProtocolParser` 做帧拆包。 |
| `networkservice.h/.cpp` | TCP 传输 + `ProtocolParser` 帧拆包。 |
| `protocolparser.h/.cpp` | A0/A3 帧的组帧、拆帧、CRC16 校验。从字节流提取完整帧（处理粘包/半包）。纯逻辑，无 Qt 依赖（仅用 QByteArray）。 |
| `modbusservice.h/.cpp` | 基于 Qt SerialBus（`QModbusTcpClient`/`QModbusRtuSerialClient`）。读保持/输入寄存器、写单/多寄存器。 |

## 架构图

```
┌──────────────────────────────────────────────────────────────┐
│  UI 层 (src/ui/pages/)                                        │
│  ConnectionPage / ProtocolDebugPage / CommandOperationPage   │
│  DeviceUpgradePage / TerminalPage                             │
│      ↓ setConnectionManager / 订阅信号                        │
├──────────────────────────────────────────────────────────────┤
│  ConnectionManager  (全局唯一，MainWindow 持有)                │
│      connect/disconnect/sendFrame/sendRaw                     │
│      信号: frameReceived / rawBytesReceived / modbusReplies   │
├───────────────────┬──────────────────┬───────────────────────┤
│  SerialService    │  NetworkService  │  ModbusService        │
│  (QSerialPort)    │  (QTcpSocket)    │  (Qt SerialBus)       │
│   + ProtocolParser│   + ProtocolParser│   TCP / RTU           │
├───────────────────┴──────────────────┘   读/写寄存器          │
│  ProtocolParser (A0/A3 帧封装 + CRC16 + 粘包拆包)             │
├──────────────────────────────────────────────────────────────┤
│  models 层 (FrameCodec — 字段表层 payload 编解码，不动)        │
└──────────────────────────────────────────────────────────────┘
```

## 一次完整的"协议调试组包发送"流程

```
ProtocolDebugPage 收集字段值
  → FrameCodec::encode(proto, values)        // 字段表层：字段值 → payload
  → ProtocolParser::buildWriteFrame(...)     // 封装层：按 proto.transport 选 A0/A3
  → ConnectionManager::sendFrame(frame)      // 传输层：串口/网口发出
  → 对端回包
  → ConnectionManager::frameReceived(frame)  // SerialService/NetworkService 已拆帧
  → ProtocolParser::parseFrame(frame)        // 去帧头取 payload
  → FrameCodec::decode(proto, payload)       // 字段表层：解 payload → 字段值
  → 刷新解帧表格
```

Modbus 协议不走 ProtocolParser，直接由页面调 `ConnectionManager::modbusService()->readHoldingRegisters(...)` 等方法，结果经 `modbusReplies` 信号回传。

## 传输封装类型（A0/A3/Modbus）

每个协议在 `.bin` 文件头 `flags` 字段的低 2 位自带传输类型（0=A0, 1=A3, 2=Modbus），向后兼容（旧文件 flags=0 即 A0）。在 Web 协议编辑器里选择，随 `.bin` 持久化。

- **A0 帧**：`[0xA0][len][src][dst][frameType][mainCmd][subCmd][payload...][crcLo][crcHi]`（CRC16 收尾，小端长度）
- **A3 帧**：`[0xA3][lenHi][lenLo][src][dst][frameType][mainCmd][subCmd][0x00][payload...][0x0D][0x0A]`（回车换行收尾，大端长度）
- **Modbus**：由 Qt SerialBus 按 Modbus TCP/RTU 规范组包

## 依赖

- Qt 模块：`SerialPort`（串口）、`SerialBus`（Modbus）、`Network`（TCP）。
- 复用 `network/SerialPortManager`（串口底层）和 `models/FrameCodec`（字段表层）。

## 接入新页面

新页面要收发数据时：
1. 头文件加 `setConnectionManager(services::ConnectionManager*)` 注入接口。
2. 构造里 `connect(m_conn, &ConnectionManager::frameReceived, ...)` 订阅需要的信号。
3. 发送调 `m_conn->sendFrame(frame)`（A0/A3）或 `m_conn->modbusService()->...`（Modbus）。
4. MainWindow 的 `ensureXxxPage()` 工厂里调 `page->setConnectionManager(m_connManager)`。
