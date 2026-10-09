# program — 5axis 工程用户程序

与 `axis.xml` / `model.xml` 等同级，专放**可加载运行的程序**（不是机床硬件配置）。

## 目录结构（按功能分类）

```text
config/5axis/program/
  README.md
  bt/       # 行为树程序（GUI / Groot / NRT LOAD）：motion.xml
  nc/       # G-code：*.nc
  plc/      # PLC 逻辑（IEC 61131-3 ST）：plc.st
```

## 当前文件

| 文件 | 类型 | 说明 |
|------|------|------|
| [`bt/motion.xml`](bt/motion.xml) | 行为树 | 命令示例子树 + 默认主树 `Cmd_NcSphereHexGrid` |
| [`nc/maple.nc`](nc/maple.nc) | NC | R40 球面法向跟随的 XYZAC 蝴蝶轮廓（保留文件名以兼容现有配置） |
| [`nc/sphere_hex_grid.nc`](nc/sphere_hex_grid.nc) | NC | 覆盖可达上半球的连续 XYZAC 六边形网格（默认主树） |
| [`nc/demo_butterfly.nc`](nc/demo_butterfly.nc) | NC | 蝴蝶轮廓 G0/G1 |
| [`nc/demo_rect.nc`](nc/demo_rect.nc) | NC | 圆 + 螺旋进中心示例 |
| [`nc/demo_sphere_carve.nc`](nc/demo_sphere_carve.nc) | NC | 上半球表面雕刻：螺旋 + 纬线 + 经线 + 极区花瓣 |
| [`nc/hexagon.nc`](nc/hexagon.nc) | NC | R40 球面六边形螺旋连续图案（6166 pt, step=0.5 mm, 45 圈） |
| [`plc/plc.st`](plc/plc.st) | ST | PLC 程序与任务配置；CMake 按构建模式注入周期，构建期编入 RT |

## 加载路径示例

```text
config/5axis/program/bt/motion.xml
config/5axis/program/nc/sphere_hex_grid.nc
```

在 GUI 行为树面板中打开 `bt/motion.xml`，再 `LOAD` / `START`。  
默认主树 `Cmd_NcSphereHexGrid`：`NcParse` 解析 `nc/sphere_hex_grid.nc` → `PathMove` 下发连续球面六边形网格。

### 连续球面六边形网格

- 覆盖 R40 球面 `0°–85°` 极角，保留 5° 赤道防碰边界。
- 包含 151 个完整六边形、498 条唯一网格边；欧拉化路由重复 158 条边，实现全程连续 `G1` 加工。
- 共 6643 个切削点，最大步长 `0.5 mm`；A 范围约 `6.2°–84.4°`，C 范围约 `-196.1°–210.0°`。
- Z 轴下限扩展到 `-0.18 m`，进给为 `5 mm/s`，满足中心区域的 C 轴速度约束。
- 可运行 `python tools/gen_sphere_hex_grid.py` 重新生成该 NC 文件。

### 蝴蝶五轴雕刻

- NC 点包含 `X/Y/Z/A/C`，A/C 以度编程，解析后以弧度进入控制器。
- 球心为 `(0, 0, 70)` mm、半径为 `40 mm`；A/C 将每点球面外法向对准固定主轴。
- 蝴蝶轮廓按 `0.5 mm` 最大平面步长细分，共 305 个切削点，球面弦高误差小于 `0.010 mm`。
- A 范围约 `6.9°–33.6°`，C 连续转动一圈，范围为 `0°–360°`。
- 可运行 `python tools/gen_butterfly_rtcp.py` 重新生成该 NC 文件。

换程序：改 `NcParse` 的 `filePath`（例如 `config/5axis/program/nc/demo_rect.nc`）。  
切换主树：改 XML 中 `main_tree_to_execute`，或在 Groot 中选择对应 `BehaviorTree ID`。

### 球表面雕刻

- 行为树：`Cmd_NcSphereCarve`（`main_tree_to_execute="Cmd_NcSphereCarve"`）
- NC：[`nc/demo_sphere_carve.nc`](nc/demo_sphere_carve.nc)
- 几何：半径 **R=40 mm** 上半球，球心程序坐标 `(0, 0, 140)` mm  
  （与 MuJoCo 工件球 `workpiece_sphere` 半径一致；路径为刀尖笛卡尔轨迹）
- 图案组成：
  1. **螺旋**：θ=8°→78°，绕 10 圈
  2. **纬线环**：θ=25° / 45° / 65° 闭合圆
  3. **经线**：φ=0° / 45° / 90° / 135°
  4. **极区花瓣**：6 瓣 spherical rosette
- A/C：该子树保持 `rx/rz=0`（刀轴竖直）；若要边雕边倾转，可改 `PathMove` 的 `endRx/endRz`
