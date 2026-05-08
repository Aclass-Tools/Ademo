# CMake Modules

Place custom CMake helper modules in this directory.

## Current modules

- `AclassProjectOptions.cmake`: common language/toolchain options.
- `AclassPlatform.cmake`: platform-specific target and install behavior.

## Why this split

- Keep root `CMakeLists.txt` minimal and readable.
- Centralize Windows/Linux/macOS differences in reusable functions.
- Scale safely when adding packaging, deployment, or platform flags later.
