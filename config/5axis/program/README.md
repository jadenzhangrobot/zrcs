# program — 5axis 工程用户程序

与 `axis.xml` / `model.xml` 等同级，专放**可加载运行的程序**（不是机床硬件配置）。

## 当前文件

| 文件 | 类型 | 说明 |
|------|------|------|
| [`motion.xml`](motion.xml) | 行为树 | 命令示例子树 + 默认主树 `Cmd_NcPath` |
| [`demo_rect.nc`](demo_rect.nc) | NC | 简单矩形 G0/G1 示例 |

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
默认主树 `Cmd_NcPath`：`NcParse` 解析本目录 `demo_rect.nc` → `PathMove` 下发 `MovePath`。  
切换主树：改 XML 中 `main_tree_to_execute`，或在 Groot 中选择对应 `BehaviorTree ID`。
