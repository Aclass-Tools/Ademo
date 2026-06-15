# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this Qt Industrial Client project.

## Project Overview

This is a Qt-based industrial client application for communicating with STM32F407VET6 embedded devices. The application provides a user interface for monitoring and controlling industrial equipment via serial port (UART) or network communication, with comprehensive protocol configuration capabilities.

## Technology Stack

**Framework:** Qt 6.8.0 (MSVC2022_64)
**Language:** C++
**Build System:** Visual Studio 2022 (v145 toolset)
**Target Platform:** Windows 64-bit
**Qt Modules:** core, gui, widgets, serialport, network

## High-Level Architecture

The application follows a layered architecture with three main components:

1. **UI Layer** - Qt Widget-based pages and delegates
2. **Service Layer** - Communication and configuration services
3. **Model Layer** - Data models and parameter definitions

### Key Architectural Relationships

- `MainWindow` is the central hub that owns all services and UI pages
- Communication flows: UI Pages → MainWindow → SerialService/NetworkService → ProtocolParser
- Callbacks use `std::function` for loose coupling between layers

## Project Structure

```
QtIndustrialClient/
├── ui/                          # UI Layer
│   ├── MainWindow               # Main application window (hub)
│   ├── ConnectionConfigPage     # Serial/Network connection setup
│   ├── ProtocolConfigPage       # Protocol configuration (editable tables)
│   ├── CommandOperationPage     # Parameter read/write operations
│   ├── ParameterPage            # Parameter display
│   ├── AddProtocolDialog        # Dialog for adding sub-commands
│   ├── AddMainOrderDialog       # Dialog for adding main commands
│   └── delegates/
│       ├── ActionButtonDelegate # Custom table action buttons
│       └── ComboBoxDelegate     # Dropdown for permission/datatype fields
├── services/                    # Service Layer
│   ├── SerialService            # UART communication (QSerialPort)
│   ├── NetworkService           # Network communication
│   ├── ProtocolParser           # Frame encoding/decoding (A0/A3 protocols + Modbus CRC)
│   └── ConfigService            # Configuration persistence (JSON)
├── models/                      # Model Layer
│   ├── ParameterRecord          # Parameter definition structure
│   ├── ParameterTableModel      # Table model for parameters
│   └── ParameterFilterModel     # Filter proxy model
├── main.cpp                     # Entry point
├── QtIndustrialClient.slnx      # Visual Studio solution
└── QtIndustrialClient.vcxproj   # Visual Studio project
```

## Core Services

### SerialService ([services/SerialService.h](services/SerialService.h))
- Manages QSerialPort for UART communication
- Uses ProtocolParser for frame encoding/decoding
- Provides callbacks for logging and frame reception
- Key methods: `openPort()`, `closePort()`, `sendFrame()`

### ProtocolParser ([services/ProtocolParser.h](services/ProtocolParser.h))
- Handles frame buffering and extraction from byte stream
- Supports A0 and A3 protocol frame types
- Implements Modbus CRC16 for error checking
- Key methods: `appendData()`, `takeAvailableFrames()`, `buildReadFrame()`, `buildWriteFrame()`
- Key struct: `ProtocolFrame` - parsed frame structure with sourceId, targetId, commands, payload

### ConfigService ([services/ConfigService.h](services/ConfigService.h))
- Loads and saves parameter configurations from JSON files
- Default config path: `document/ceg_config.json`
- Key methods: `loadRecords()`, `saveRecords()`, `findDefaultConfigPath()`

## Main UI Pages

### MainWindow ([ui/MainWindow.h](ui/MainWindow.h))
- Owns all UI pages and communication services
- Manages connection state (Serial/Network/None)
- Central log view for communication debugging
- Coordinates page switching via QStackedWidget

### ConnectionConfigPage ([ui/ConnectionConfigPage.h](ui/ConnectionConfigPage.h))
- Dual-mode configuration: Serial or Ethernet
- Serial: Port selection, baud rate, board/parent IDs
- Ethernet: IP address, port configuration

### ProtocolConfigPage ([ui/ProtocolConfigPage.h](ui/ProtocolConfigPage.h))
- **Two-level tab structure**: Protocol types (A0/A3) → Main command groups
- Editable table with 15 columns for parameter configuration
- Cell merging for command-level fields (spans multiple sub-command rows)
- **Main command operations**:
  - Add: Create new main command group with unique ID (>1)
  - Delete: Remove entire main command and all its sub-commands
  - Rename: Double-click tab to rename main command
- **Sub-command operations**:
  - Add: Open dialog to add new parameter rows
  - Delete: Remove selected sub-command (handles merged cells)
  - Clear all: Empty entire configuration
- **Configuration management**:
  - Save to JSON file (with duplicate sub-command validation)
  - Load from JSON file
- Uses ComboBoxDelegate for permission (R/W/RW) and data type fields
- Table selection mode: SelectRows for better UX

### CommandOperationPage ([ui/CommandOperationPage.h](ui/CommandOperationPage.h))
- Tabbed interface organized by parameter groups
- Per-parameter Read/Write action buttons
- Real-time value updates from device
- Uses ActionButtonDelegate for in-table buttons

## Data Models

### ParameterRecord ([models/ParameterRecord.h](models/ParameterRecord.h))
```cpp
struct ParameterRecord {
    QString protocolType;      // "A0" or "A3"
    QString groupName;         // Group display name
    int groupId;                // Group identifier
    QString commandName;        // Command name
    int commandId;              // Command identifier
    QString permission;         // "Read", "Write", "ReadWrite"
    int readIndex;              // Offset in payload
    QString parameterName;      // Parameter display name
    QString dataType;           // Data type (int, float, enum, etc.)
    int dataLength;             // Byte length
    QString description;
    QString defaultValue;
    QString unit;
    QString storageLocation;
    QString maxValue, minValue, step;  // Validation
    QStringList enumValues;     // For enum types
    QString remark;
};
```

## Build and Run

**Building:**
1. Open `QtIndustrialClient.slnx` in Visual Studio 2022
2. Select Debug or Release configuration
3. Build solution (F7)

**Running:**
- Debug: F5 in Visual Studio
- Release: Run `x64/Release/QtIndustrialClient.exe`

**Qt Requirements:**
- Qt 6.8.0 (MSVC2022_64) must be installed
- Qt VS Tools extension recommended for Visual Studio

## Recent Changes

- Added main command (主指令) add/delete/rename functionality
- Added sub-command (子指令) add/delete operations
- Added clear all configuration button
- Added duplicate sub-command validation on save
- Added double-click tab renaming for main commands
- Table selection behavior changed to SelectRows for better UX
- Enhanced protocol config table with cell merging for command fields
- Fixed config save/load with proper merged cell handling

## Important Notes

- The application uses Qt Widgets for UI
- Communication protocols: A0 and A3 frame types with Modbus CRC16
- Protocol response type for read commands is 0x54
- All UI forms use Qt Designer `.ui` files
- Callbacks use `std::function` instead of Qt signals/slots in some places for flexibility
- Configuration file: `document/ceg_config.json`
- Main command IDs must be greater than 1
- Sub-command IDs must be unique within their protocol/group
