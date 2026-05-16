#include "core/MainWindow.h"
#include <QPainter>

StatusIndicator::StatusIndicator(QWidget *parent)
    : QWidget(parent), currentState(Idle), statusText("就绪")
{
}

void StatusIndicator::setState(State state)
{
    currentState = state;
    switch (state) {
        case Idle:
            statusText = "空闲";
            break;
        case Running:
            statusText = "运行中";
            break;
        case Alarm:
            statusText = "报警";
            break;
        case EStop:
            statusText = "急停";
            break;
    }
    update();
}

void StatusIndicator::setText(const QString &text)
{
    statusText = text;
    update();
}

QColor StatusIndicator::getColor() const
{
    switch (currentState) {
        case Idle:
            return QColor(100, 200, 100);  // 绿色
        case Running:
            return QColor(100, 150, 255);  // 蓝色
        case Alarm:
            return QColor(255, 200, 0);    // 黄色
        case EStop:
            return QColor(255, 80, 80);    // 红色
    }
    return QColor(100, 100, 100);
}

void StatusIndicator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor color = getColor();
    const int margin = 10;
    const int diameter = qMin(width(), height()) - margin * 2;
    const int x = (width() - diameter) / 2;
    const int y = (height() - diameter) / 2;
    
    // 绘制外圆
    painter.setBrush(color);
    painter.setPen(QPen(color.darker(150), 3));
    painter.drawEllipse(x, y, diameter, diameter);
    
    // 绘制内圆（脉冲效果）
    if (currentState == Running) {
        painter.setBrush(color.lighter(120));
        painter.setPen(Qt::NoPen);
        const int innerDiameter = diameter * 3 / 5;
        const int innerX = (width() - innerDiameter) / 2;
        const int innerY = (height() - innerDiameter) / 2;
        painter.drawEllipse(innerX, innerY, innerDiameter, innerDiameter);
    }
    
    // 绘制文字
    painter.setPen(palette().color(QPalette::WindowText));
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter, statusText);
}
