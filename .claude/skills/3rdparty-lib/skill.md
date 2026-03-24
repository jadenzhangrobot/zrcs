---
name: 3rdparty-lib
description: zrcs 项目第三方库管理规范。添加、更新或排查第三方依赖时使用，涵盖引入方式优先级、CMake 集成和目录结构。
---

# 第三方库管理规范

zrcs 项目的第三方库统一放置在 `3rdParty/` 目录下，通过 `cmake/3rdParty.cmake` 集成到构建系统。

## 当前已有的第三方库

| 库名 | 引入方式 | 说明 |
|------|---------|------|
| tinyxml2 | `3rdParty/tinyxml2` + `add_subdirectory` | XML 解析 |
| ruckig | `3rdParty/ruckig` + `add_subdirectory` | 实时轨迹规划 |
| spdlog | `3rdParty/spdlog` + `add_subdirectory` | 日志库 |
| BehaviorTree.CPP | `3rdParty/Groot/depend/BehaviorTree.CPP` | 行为树（作为 Groot 子模块） |
| Groot | `3rdParty/Groot` + `add_subdirectory` | 行为树可视化编辑器 |
| Trajectory_planning | `3rdParty/Trajectory_planning` | 轨迹规划算法 |
| gpr | `3rdParty/gpr` | 高斯过程回归 |
| Boost | `find_package(Boost)` | MSYS2 UCRT64 系统安装 |
| Protobuf | `find_package(Protobuf)` | MSYS2 UCRT64 系统安装 |
| Abseil | `find_package(absl)` | MSYS2 UCRT64 系统安装（Protobuf 依赖） |
| ZeroMQ + cppzmq | `find_package(cppzmq)` | MSYS2 UCRT64 系统安装 |
| Qt5 | `find_package(Qt5)` | MSYS2 UCRT64 系统安装 |

## 添加新第三方库的流程

### 步骤 1：确定引入方式（按优先级排列）

1. **MSYS2 UCRT64 pacman 安装**：先检查是否有对应的 UCRT64 包
   ```bash
   D:/msys2/usr/bin/pacman.exe -Ss mingw-w64-ucrt-x86_64-<库名>
   ```
   如果有，用 pacman 安装，CMake 中用 `find_package()` 引入。

2. **git clone 到 `3rdParty/`**：如果 pacman 没有，或者需要特定版本/定制编译选项
   ```bash
   cd 3rdParty
   git clone --depth 1 --branch <版本tag> https://github.com/<repo>.git <库名>
   ```
   - **必须** shallow clone（`--depth 1`）减少仓库体积
   - **必须**指定稳定版本 tag，不要用 main/master

3. **header-only 库**：如果库是纯头文件，clone 后只需 `include_directories` 即可，不需要 `add_subdirectory`

### 步骤 2：CMake 集成

在 `cmake/3rdParty.cmake` 中添加（保持现有格式）：

```cmake
# 添加<库名>
set(<LIB>_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)  # 关闭不需要的构建选项
set(<LIB>_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/<库名>)
```

**注意事项**：
- 使用 `FORCE` 关闭示例、测试、工具的构建选项，减少编译时间
- 全局 `BUILD_SHARED_LIBS` 已设为 `OFF`，默认静态链接
- 如果库的 CMake 目标名不直观，加注释说明

### 步骤 3：在目标子项目中链接

在对应子项目的 `CMakeLists.txt` 中：

```cmake
# 添加 include 路径
include_directories(
    ${CMAKE_SOURCE_DIR}/3rdParty/<库名>/include
)

# 链接库
target_link_libraries(<target> PRIVATE
    <库的cmake目标名>   # 如 spdlog::spdlog, tinyxml2 等
)
```

### 步骤 4：网络问题处理

如果 `git clone` 因网络问题失败，按顺序尝试：

1. 清除代理重试：`git -c http.proxy="" clone ...`
2. 使用镜像代理：`https://ghproxy.net/https://github.com/...`
3. 让用户手动下载

## 关键约束

- **不要**在 `3rdParty/` 中提交 `.pyc`、`__pycache__`、编译产物等
- **不要**修改第三方库的源码。如果需要 patch，在 `cmake/` 中用 CMake 方式处理（如 `target_compile_definitions`）
- 如果第三方库有子模块，确保文档说明需要 `--recursive` clone
- 所有第三方库**静态链接**（`BUILD_SHARED_LIBS OFF` 已在 `3rdParty.cmake` 全局设置）

## 相关文件

- [cmake/3rdParty.cmake](cmake/3rdParty.cmake) — 第三方库 CMake 集成入口
- [CMakeLists.txt](CMakeLists.txt) — 顶层构建，`include(3rdParty.cmake)` 和 `MOTION_CONTROL_LIBS` 定义
- [3rdParty/](3rdParty/) — 第三方库源码目录
