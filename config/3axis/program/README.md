# program — 3axis 工程用户程序

与 `axis.xml` / `model.xml` 等同级，专放**可加载运行的程序**（不是机床硬件配置）。

## 目录结构（按功能分类）

```text
config/3axis/program/
  README.md
  bt/       # 行为树程序（GUI / Groot / NRT LOAD）：motion.xml
  nc/       # G-code：*.nc
  plc/      # PLC 逻辑（IEC 61131-3 ST）：*.st
```

## 当前文件

| 文件 | 类型 | 说明 |
|------|------|------|
| [`bt/motion.xml`](bt/motion.xml) | 行为树 | 命令示例子树 + 默认主树 `Cmd_NcPath`（NcParse → PathMove） |
| [`nc/demo_rect.nc`](nc/demo_rect.nc) | NC | 简单矩形 G0/G1 示例 |

## 加载路径示例

```text
config/3axis/program/bt/motion.xml
config/3axis/program/nc/demo_rect.nc
```

在 GUI 行为树面板中打开 `bt/motion.xml`，再 `LOAD` / `START`。  
切换主树：改 XML 中 `main_tree_to_execute`，或在 Groot 中选择对应 `BehaviorTree ID`。
