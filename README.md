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
│  ├─ mainwindow.cpp
│  └─ mainwindow.h
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

## Scripted Build Workflow (PowerShell)

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
