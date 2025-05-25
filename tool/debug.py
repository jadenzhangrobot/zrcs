import sys
import re
import matplotlib.pyplot as plt

# 初始化交互模式
plt.ion()
fig, ax = plt.subplots()
x, y = [], []
line, = ax.plot(x, y, 'b-')

# 自定义输出重定向
class PlotStream:
    def write(self, text):
        # 从输出中提取数值（示例：匹配 "DATA: 42.3"）
        match = re.search(r'DATA:\s*([\d.]+)', text)
        if match:
            new_value = float(match.group(1))
            x.append(len(x))  # 用序号作为 X 轴
            y.append(new_value)
            
            # 更新图表
            line.set_xdata(x)
            line.set_ydata(y)
            ax.relim()
            ax.autoscale_view()
            fig.canvas.draw()
            fig.canvas.flush_events()

# 替换标准输出
sys.stdout = PlotStream()

# 示例调试代码（模拟产生数据）
import time, random
for _ in range(100):
    value = random.uniform(0, 100)
    print(f"DATA: {value:.2f}")  # 关键输出格式
    time.sleep(0.1)