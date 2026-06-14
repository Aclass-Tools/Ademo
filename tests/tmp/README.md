# tests/tmp — 协议层端到端测试

> 目录名沿用初建时的“临时”命名，实际为长期保留的协议层自测工程，已纳入仓库。

## 用途

独立于主程序构建的命令行测试程序，验证模型层（`BinProtocolLoader` / `FrameCodec`）
与 Python 生成脚本 `scripts/generate_sample_bin.py` 的二进制格式**三方一致**：

1. Qt 能正确加载 Python 生成的 `data/protocols/sample_protocol.bin`
2. `FrameCodec::encode` → `decode` 往返一致（小端整数 / 位字段 / 浮点 / Struct 嵌套）
3. `BinProtocolLoader::save` → 重新 `load` 往返一致

成功输出 `ALL OK`，失败返回非 0。

## 构建与运行

```powershell
# 配置（指向 Qt 6.8.3 / MinGW）
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" -S tests/tmp -B build/test -G Ninja `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64 `
    -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
    -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe

# 构建
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/test

# 运行（需 Qt6Core.dll，首次用 windeployqt 部署一次）
& "C:\Qt\6.8.3\mingw_64\bin\windeployqt.exe" --debug build\test\binproto_test.exe
build\test\binproto_test.exe
```

> 注意：测试源码引用 `${CMAKE_SOURCE_DIR}/../../src/models/*.cpp`，依赖主程序源码，
> 修改主程序模型层后需重新构建此测试以确认未回归。

## 文件

- `test_binprotocol_main.cpp` — 测试主程序（用 `std::printf` 输出，避免 GUI 子系统下 qDebug 无控制台）
- `CMakeLists.txt` — 独立构建配置，链接 Qt6::Core
