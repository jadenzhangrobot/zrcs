# IPC Project

这是一个使用C++17标准和CMake构建系统的项目。

## 构建说明

### Windows (使用Visual Studio)

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
```

## 运行

构建完成后，可执行文件位于 `build/bin/` 目录下。

```bash
# Windows
.\build\bin\Release\ipc_project.exe

# Linux/macOS
./build/bin/ipc_project
```

## 项目结构

```
ipc/
├── CMakeLists.txt          # 主CMake配置文件
├── README.md               # 项目说明
└── src/                    # 源代码目录
    ├── CMakeLists.txt      # 源码CMake配置
    ├── main.cpp            # 主程序入口
    ├── utils.hpp           # 工具函数头文件
    └── utils.cpp           # 工具函数实现
```

## C++17特性

本项目使用了以下C++17特性：
- 结构化绑定
- `std::optional`
- `std::string_view`
- `if constexpr`
- 字符串字面量后缀

## 开发环境要求

- CMake 3.10 或更高版本
- 支持C++17的编译器（GCC 7+, Clang 5+, MSVC 2017+）