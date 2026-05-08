# Dist

Distribution artifacts are organized into fixed folders under project root `dist/`:

- `dist/debug/`: deployed files from Debug builds
- `dist/release/`: deployed files from Release builds
- `dist/installer/`: installer packages

## Targets

- `dist_clean`: clear and recreate `dist/debug`, `dist/release`, `dist/installer`
- `dist_win`: deploy runtime files with `windeployqt`
- `dist_installer`: generate NSIS installer via CPack (Release recommended)

## Installer shortcut behavior

The NSIS installer shows a wizard checkbox for desktop shortcut creation.
Start menu and desktop shortcuts are created explicitly to:

- `$INSTDIR\\AclassTool.exe`

This is handled by:

- `CPACK_NSIS_MODIFY_PATH`
- `CPACK_NSIS_CREATE_ICONS_EXTRA`
- `CPACK_NSIS_DELETE_ICONS_EXTRA`
- custom template: `cmake/Internal/CPack/NSIS.InstallOptions.ini.in`

Note:
- If you change these settings, regenerate installer after clearing old CPack cache.
- PATH options are intentionally hidden in installer UI; only desktop shortcut checkbox is shown.

## Configure paths

Set in `.vscode/settings.json` under `cmake.configureSettings`:

```json
"WINDEPLOYQT_EXECUTABLE": "C:/Qt/6.8.3/mingw_64/bin/windeployqt.exe",
"NSIS_MAKENSIS_EXECUTABLE": "C:/Program Files (x86)/NSIS/makensis.exe"
```

## Build commands

```powershell
cmake --build <build-dir> --target dist_clean
cmake --build <build-dir> --target dist_win
cmake --build <build-dir> --target dist_installer
```
