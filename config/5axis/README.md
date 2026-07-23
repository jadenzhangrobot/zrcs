# 5axis — XYZAC 五轴机床配置

## 结构

采用常见 **table-table（摇篮转台 / TRT）** 五轴布局：

| 轴 | 类型 | 含义 | 对应物理轴 |
|----|------|------|------------|
| X  | prismatic | 左右平移 | axisId 0 |
| Y  | prismatic | 前后平移 | axisId 1 |
| Z  | prismatic | 上下平移 | axisId 2 |
| A  | revolute  | 绕 X 倾转 | axisId 3 |
| C  | revolute  | 绕 Z 回转 | axisId 4 |

简化运动链：

```text
toolPose = baseTf * Trans(X) * Trans(Y) * Trans(Z) * RotX(A) * RotZ(C) * toolTf
```

参考：

- [LinuxCNC 5-axis kinematics (xyzac-trt)](https://www.linuxcnc.org/docs/devel/html/motion/5-axis-kinematics.html)
- 常见 XYZAC 转台机床：直线三轴在主轴侧或工作台侧，A/C 装在工件侧

## 文件

| 文件 | 作用 |
|------|------|
| `model.xml` | 5DOF 笛卡尔运动学模型 |
| `axis.xml`  | 逻辑轴行程 / 速度 / 加速度 |
| `servo.xml` | 5 个伺服从站映射 |
| `ethercat.xml` | EtherCAT 拓扑（沿用原文件） |
| `mujoco.xml` | MuJoCo 仿真绑定（slaveId ↔ joint） |
| `mujoco/robot.xml` | MuJoCo 五轴机床 MJCF 模型 |

## 单位

- 直线轴：米 (m)
- 旋转轴：弧度 (rad)
- 姿态 `rx/ry/rz`：弧度

## 切换项目

编辑 `config/project.txt`：

```text
5axis
```

## MuJoCo 仿真

结构：**龙门 XYZ + 床身摇篮转台 AC**（table-table / TRT）

```text
machine_base
├── trunnion_a  [a_hinge, hinge about X]
│   └── table_c [c_hinge, hinge about Z]
│       └── workpiece
└── y_gantry    [y_slide]
    └── x_carriage [x_slide]
        └── z_head [z_slide] → tool_tip
```

| slaveId | 逻辑轴 | MuJoCo joint | 类型 | 行程 |
|--------:|--------|--------------|------|------|
| 0 | X | `x_slide` | slide | ±0.40 m |
| 1 | Y | `y_slide` | slide | ±0.30 m |
| 2 | Z | `z_slide` | slide | -0.18 ~ 0.40 m |
| 3 | A | `a_hinge` | hinge | ±1.92 rad (±110°) |
| 4 | C | `c_hinge` | hinge | ±2π rad |

控制器单位与 MuJoCo 一致（m / rad），故 `qposScale=1.0`。

独立预览（需本机 MuJoCo `simulate`）：

```bash
simulate config/5axis/mujoco/robot.xml
```

## 可调参数

1. **刀具长度**：`model.xml` → `toolFrame.z`
2. **转台偏心**：写入 `baseFrame` 或关节 `offset`（当前按零偏心简化）
3. **行程/速度**：按实机改 `axis.xml`
4. **编码器比例**：按实机改 `servo.xml` 的 `encoderCountPerUnit`
5. **仿真外观/关节**：改 `mujoco/robot.xml` 几何与 `range`

## 限制

当前使用 `CartesianRobot` 通用笛卡尔模型：

- FK 对正交 XYZAC 可用
- IK 用 ZYX 欧拉角提取旋转，是近似解
- 未建模 A/C 轴交叉偏心、主轴侧双旋转（head-head）等专用机床运动学
- MuJoCo 模型用于可视化与伺服仿真，不模拟切削/材料去除

若后续要做精确五轴后处理 / TCP 控制，建议新增专用机床运动学类型。

## 参考

- [MuJoCo Modeling](https://mujoco.readthedocs.io/en/stable/modeling.html)
- [MuJoCo XML Reference](https://mujoco.readthedocs.io/en/stable/XMLreference.html)
- [LinuxCNC 5-axis kinematics](https://linuxcnc.org/docs/html/motion/5-axis-kinematics.html)
- [5-axis machine configurations](https://5-axis.org/machine-configurations-and-styles/)
