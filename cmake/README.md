# CMake 配置说明

本项目使用模块化的 CMake 配置，将不同功能的配置分离到独立的模块文件中。

## 文件结构

```
cmake/
├── compiler.cmake   # 编译器配置
├── 3rdParty.cmake   # 第三方库配置
├── realtime.cmake   # Xenomai 实时系统配置
├── ethercat.cmake   # EtherCAT 配置
├── tool.cmake       # 工具和脚本配置
└── README.md        # 本说明文件
```

## 构建选项

### 基本选项
- `realtime`: 启用 Xenomai 实时支持 (默认: OFF)
- `simulation`: 启用仿真模式 (默认: OFF)
- `test`: 构建测试程序 (默认: ON)
- `ethercat`: 启用 EtherCAT 支持 (默认: OFF)

### 构建示例

#### 标准构建
```bash
mkdir build && cd build
cmake ..
make
```

#### 实时构建
```bash
mkdir build && cd build
cmake -Drealtime=ON -Dethercat=ON ..
make
```

#### 仿真构建
```bash
mkdir build && cd build
cmake -Dsimulation=ON ..
make
```

## 模块说明

### compiler.cmake
- 设置 C++17 标准
- 配置编译器特定的标志
- 设置调试和发布模式的编译选项

### 3rdParty.cmake
- 配置 tinyxml2 库
- 配置 ruckig 库
- 查找 Boost 库（可选）
- 配置 CoppeliaSim（仅在仿真模式下）
- 配置 ZMQ 库（如果存在）

### realtime.cmake
- 查找 Xenomai 安装
- 获取 Xenomai 编译和链接标志
- 应用实时系统配置

### ethercat.cmake
- 查找 EtherCAT 库
- 配置 EtherCAT 包含目录和链接库
- 创建 EtherCAT 导入目标

### tool.cmake
- 查找 Python3 解释器
- 执行配置脚本
- 处理代码生成工具

## 依赖要求

### 必需依赖
- CMake 3.22.1+
- C++17 兼容编译器
- Python3

### 可选依赖
- Xenomai (实时模式)
- EtherLab EtherCAT Master (EtherCAT 模式)
- CoppeliaSim (仿真模式)
- Boost (可选)

## 故障排除

### Xenomai 相关问题
1. 确保 Xenomai 正确安装
2. 检查 xeno-config 是否在 PATH 中
3. 验证用户权限

### EtherCAT 相关问题
1. 确保 EtherLab EtherCAT Master 已安装
2. 检查 /opt/etherlab 路径
3. 验证库文件权限

### CoppeliaSim 相关问题
1. 检查安装路径是否正确
2. 确保 remoteApi 库存在
3. 验证头文件路径