# XLeRobot 配置工程

面向 [XLeRobot](https://github.com/Vector-Wangel/XLeRobot) 双臂移动机器人的 zrcs 项目配置。

## 目录

```
config/XLeRobot/
├── axis.xml          # 14 逻辑轴（双臂 + 夹爪 + 云台，删除底盘）
├── servo.xml         # 14 虚拟/仿真伺服
├── model.xml         # 运动学占位模型
├── mujoco.xml        # MuJoCo joint 绑定
└── mujoco/
    ├── scene.xml     # 场景入口（含地板灯光）
    ├── xlerobot.xml  # 机器人 MJCF（基于官方模型微调）
    └── assets/       # STL 网格
```

## 轴映射

| axisId | 名称 | MuJoCo joint | 说明 |
|--------|------|--------------|------|
| 0-5 | left_* | Rotation_L … Jaw_L | 左臂 SO-101 |
| 6-11 | right_* | Rotation_R … Jaw_R | 右臂 SO-101 |
| 12 | head_pan | head_pan_joint | 云台偏航 |
| 13 | head_tilt | head_tilt_joint | 云台俯仰 |

## 启用方式

编辑 `config/project.txt`：

```
XLeRobot
```

以仿真模式启动 RT（存在 `mujoco.xml` 时 HardwareFactory 会走 MuJoCo）。

## 说明 / 后续

- 网格与 MJCF 来自官方仓库 `simulation/mujoco`。
- `xlerobot.xml` 以官方 `simulation/mujoco/xlerobot.xml` 为基准，未引入额外摄像头、RASKOG 推车或 tophead/topbase 外观 mesh。
- 相对官方做了三点适配：
  1. 删除底盘 planar/wheel 控制关节，仅保留双臂、夹爪与云台控制关节；
  2. 为云台补 position actuator；
  3. 降低 position actuator 刚度，并关闭机器人复杂 mesh 碰撞，减少静止抖动。
- `model.xml` 目前是轴级 `cartesian` 占位模型，确保当前 `ModelFactory`
  能注册 `XLeRobot`；精确双臂/夹爪 IK 需后续按 SO-101 参数补齐。
- 实机 Feetech STS3215 的 `posFactor` / 方向需标定后改 `servo.xml`。
