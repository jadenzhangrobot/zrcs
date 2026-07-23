# program — 5axis 工程用户程序

与 `axis.xml` / `model.xml` 等同级，专放**可加载运行的程序**（不是机床硬件配置）。

## 当前文件

| 文件 | 类型 | 说明 |
|------|------|------|
| [`motion.xml`](motion.xml) | 行为树 | 命令示例子树 + 默认主树 `Cmd_NcMapleCarve` |
| [`maple.nc`](maple.nc) | NC | R40 球面法向跟随的 XYZAC 蝴蝶轮廓（默认主树，保留文件名以兼容现有配置） |
| [`demo_butterfly.nc`](demo_butterfly.nc) | NC | 蝴蝶轮廓 G0/G1 |
| [`demo_rect.nc`](demo_rect.nc) | NC | 圆 + 螺旋进中心示例 |
| [`demo_sphere_carve.nc`](demo_sphere_carve.nc) | NC | 上半球表面雕刻：螺旋 + 纬线 + 经线 + 极区花瓣 |
| [`hexagon.nc`](hexagon.nc) | NC | R40 球面六边形螺旋连续图案（6166 pt, step=0.5 mm, 45 圈） |

## 目录约定

```text
config/5axis/program/
  *.xml     # 行为树程序（GUI / Groot / NRT LOAD）
  *.nc      # G-code
```

## 加载路径示例

```text
config/5axis/program/motion.xml
config/5axis/program/demo_rect.nc
```

在 GUI 行为树面板中打开 `motion.xml`，再 `LOAD` / `START`。  
默认主树 `Cmd_NcMapleCarve`：`NcParse` 解析本目录 `maple.nc` → `PathMove` 下发球面蝴蝶轨迹。

### 蝴蝶五轴雕刻

- NC 点包含 `X/Y/Z/A/C`，A/C 以度编程，解析后以弧度进入控制器。
- 球心为 `(0, 0, 70)` mm、半径为 `40 mm`；A/C 将每点球面外法向对准固定主轴。
- 蝴蝶轮廓按 `0.5 mm` 最大平面步长细分，共 305 个切削点，球面弦高误差小于 `0.010 mm`。
- A 范围约 `6.9°–33.6°`，C 连续转动一圈，范围为 `0°–360°`。
- 可运行 `python tools/gen_butterfly_rtcp.py` 重新生成该 NC 文件。
换程序：改 `NcParse` 的 `filePath`（例如 `demo_rect.nc` / `demo_sphere_carve.nc`）。  
切换主树：改 XML 中 `main_tree_to_execute`，或在 Groot 中选择对应 `BehaviorTree ID`。

### 球表面雕刻

- 行为树：`Cmd_NcSphereCarve`（`main_tree_to_execute="Cmd_NcSphereCarve"`）
- NC：[`demo_sphere_carve.nc`](demo_sphere_carve.nc)
- 几何：半径 **R=40 mm** 上半球，球心程序坐标 `(0, 0, 140)` mm  
  （与 MuJoCo 工件球 `workpiece_sphere` 半径一致；路径为刀尖笛卡尔轨迹）
- 图案组成：
  1. **螺旋**：θ=8°→78°，绕 10 圈
  2. **纬线环**：θ=25° / 45° / 65° 闭合圆
  3. **经线**：φ=0° / 45° / 90° / 135°
  4. **极区花瓣**：6 瓣 spherical rosette
- A/C：该子树保持 `rx/rz=0`（刀轴竖直）；若要边雕边倾转，可改 `PathMove` 的 `endRx/endRz`
