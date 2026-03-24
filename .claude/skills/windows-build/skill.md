---
name: windows-build
description: Windows 平台构建规范。强制使用 MSYS2 UCRT64 工具链编译 zrcs 项目，确保 CMake、编译器和依赖库路径正确。
---

# Windows 构建规范 — MSYS2 UCRT64

在 Windows 系统上编译 zrcs 项目时，**必须**使用 MSYS2 的 **UCRT64** 环境，不得使用 MinGW64、MSVC 或其他工具链。

## 为什么选择 UCRT64

- 使用 Windows 原生 **Universal C Runtime (UCRT)**，与现代 Windows 10/11 系统兼容性更好
- 相比 MinGW64 (MSVCRT)，UCRT 对 C99/C11 标准支持更完善
- Qt5、Protobuf、ZMQ 等依赖均可通过 `pacman` 在 UCRT64 环境下统一管理

## 核心规则

### 1. 工具链路径

所有编译工具和库的前缀路径为 `D:/msys2/ucrt64`（或用户自定义的 MSYS2 安装路径下的 `ucrt64`）：

| 项目 | 路径 |
|------|------|
| C 编译器 | `D:/msys2/ucrt64/bin/gcc.exe` |
| C++ 编译器 | `D:/msys2/ucrt64/bin/g++.exe` |
| CMake | `D:/msys2/ucrt64/bin/cmake.exe`（或系统 CMake） |
| Make | `D:/msys2/ucrt64/bin/mingw32-make.exe` 或 `ninja` |
| Qt5 | `D:/msys2/ucrt64/lib/cmake/Qt5` |
| 库搜索路径 | `D:/msys2/ucrt64/lib` |
| 头文件路径 | `D:/msys2/ucrt64/include` |

### 2. CMake 配置规范

在 CMakeLists.txt 中设置 Qt5 和依赖路径时，**必须**指向 `ucrt64`：

```cmake
# 正确 — 使用 UCRT64
set(CMAKE_PREFIX_PATH "D:/msys2/ucrt64")
set(Qt5_DIR "D:/msys2/ucrt64/lib/cmake/Qt5")

# 错误 — 不要使用 mingw64
# set(CMAKE_PREFIX_PATH "D:/msys2/mingw64")  # WRONG
```

### 3. CMake 构建命令

```bash
# 配置（从项目根目录）
cmake -B build -G "MinGW Makefiles" \
  -DCMAKE_C_COMPILER=D:/msys2/ucrt64/bin/gcc.exe \
  -DCMAKE_CXX_COMPILER=D:/msys2/ucrt64/bin/g++.exe \
  -DCMAKE_PREFIX_PATH=D:/msys2/ucrt64 \
  -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build -j$(nproc)
```

或使用 Ninja（更快）：

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=D:/msys2/ucrt64/bin/gcc.exe \
  -DCMAKE_CXX_COMPILER=D:/msys2/ucrt64/bin/g++.exe \
  -DCMAKE_PREFIX_PATH=D:/msys2/ucrt64 \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build -j$(nproc)
```

### 4. MSYS2 依赖安装

所有依赖必须安装 `mingw-w64-ucrt-x86_64-` 前缀的包，**不要**安装 `mingw-w64-x86_64-`（那是 MinGW64 的包）：

```bash
# 正确 — UCRT64 包
pacman -S mingw-w64-ucrt-x86_64-qt5-base
pacman -S mingw-w64-ucrt-x86_64-protobuf
pacman -S mingw-w64-ucrt-x86_64-zeromq
pacman -S mingw-w64-ucrt-x86_64-cppzmq
pacman -S mingw-w64-ucrt-x86_64-abseil-cpp
pacman -S mingw-w64-ucrt-x86_64-cmake
pacman -S mingw-w64-ucrt-x86_64-ninja
pacman -S mingw-w64-ucrt-x86_64-vtk
pacman -S mingw-w64-ucrt-x86_64-opencascade

# 错误 — 这些是 MinGW64 的包，不要装
# pacman -S mingw-w64-x86_64-qt5-base  # WRONG
```

### 5. 环境变量

确保 `PATH` 中 UCRT64 优先于其他 MSYS2 环境：

```bash
export PATH="/ucrt64/bin:$PATH"
```

或在 Windows 系统环境变量中将 `D:\msys2\ucrt64\bin` 放在最前面。

### 6. 代码中禁止硬编码路径

- CMakeLists.txt 中如需引用 MSYS2 路径，使用 `CMAKE_PREFIX_PATH`，避免将 `D:/msys2/ucrt64` 散布到多处。
- 优先使用 `find_package()` 自动发现，而非手动指定路径。

### 7. 修改现有代码时的注意事项

当前 [zrcsGui/CMakeLists.txt](zrcsGui/CMakeLists.txt) 中存在硬编码的 MinGW64 路径：
```cmake
set(CMAKE_PREFIX_PATH "D:/msys2/mingw64")       # 需改为 ucrt64
set(Qt5_DIR "D:/msys2/mingw64/lib/cmake/Qt5")   # 需改为 ucrt64
```

修改时将 `mingw64` 替换为 `ucrt64`。
