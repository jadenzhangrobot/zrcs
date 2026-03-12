#include "core/mainwindow_refactored.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>

StatusIndicator::StatusIndicator(QWidget *parent)
    : QWidget(parent), currentState(Idle), statusText("就绪")
{
    setFixedSize(120, 120);
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
    
    // 绘制外圆
    painter.setBrush(color);
    painter.setPen(QPen(color.darker(150), 3));
    painter.drawEllipse(10, 10, 100, 100);
    
    // 绘制内圆（脉冲效果）
    if (currentState == Running) {
        painter.setBrush(color.lighter(120));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(30, 30, 60, 60);
    }
    
    // 绘制文字
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, statusText);
}

AxisPositionDisplay::AxisPositionDisplay(const QString &axisName, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    
    QLabel *titleLabel = new QLabel(QString("<b>%1 轴</b>").arg(axisName));
    titleLabel->setStyleSheet("color: #FFD700; font-size: 12pt;");
    layout->addWidget(titleLabel);
    
    // 坐标显示
    QHBoxLayout *posLayout = new QHBoxLayout();
    posLayout->addWidget(new QLabel("机械:"));
    machineLabel = new QLabel("0.000");
    machineLabel->setStyleSheet("color: #00FF00; font-family: monospace; font-size: 11pt;");
    posLayout->addWidget(machineLabel);
    posLayout->addWidget(new QLabel("绝对:"));
    absoluteLabel = new QLabel("0.000");
    absoluteLabel->setStyleSheet("color: #00FF00; font-family: monospace; font-size: 11pt;");
    posLayout->addWidget(absoluteLabel);
    posLayout->addWidget(new QLabel("相对:"));
    relativeLabel = new QLabel("0.000");
    relativeLabel->setStyleSheet("color: #00FF00; font-family: monospace; font-size: 11pt;");
    posLayout->addWidget(relativeLabel);
    layout->addLayout(posLayout);
    
    // 动力学数据
    QHBoxLayout *dynLayout = new QHBoxLayout();
    dynLayout->addWidget(new QLabel("速度:"));
    velocityLabel = new QLabel("0.0");
    velocityLabel->setStyleSheet("color: #87CEEB; font-family: monospace;");
    dynLayout->addWidget(velocityLabel);
    dynLayout->addWidget(new QLabel("加速度:"));
    accelerationLabel = new QLabel("0.0");
    accelerationLabel->setStyleSheet("color: #87CEEB; font-family: monospace;");
    dynLayout->addWidget(accelerationLabel);
    layout->addLayout(dynLayout);
    
    // 伺服数据
    QHBoxLayout *servoLayout = new QHBoxLayout();
    servoLayout->addWidget(new QLabel("扭矩:"));
    torqueLabel = new QLabel("0%");
    torqueLabel->setStyleSheet("color: #FF6347; font-family: monospace;");
    servoLayout->addWidget(torqueLabel);
    servoLayout->addWidget(new QLabel("跟随误差:"));
    followErrorLabel = new QLabel("0.0");
    followErrorLabel->setStyleSheet("color: #FF6347; font-family: monospace;");
    servoLayout->addWidget(followErrorLabel);
    servoLayout->addWidget(new QLabel("温度:"));
    tempLabel = new QLabel("0°C");
    tempLabel->setStyleSheet("color: #FF6347; font-family: monospace;");
    servoLayout->addWidget(tempLabel);
    layout->addLayout(servoLayout);
    
    setStyleSheet("background-color: #1a1a1a; border: 1px solid #444; border-radius: 5px;");
}

void AxisPositionDisplay::updatePosition(double machine, double absolute, double relative)
{
    machineLabel->setText(QString::number(machine, 'f', 3));
    absoluteLabel->setText(QString::number(absolute, 'f', 3));
    relativeLabel->setText(QString::number(relative, 'f', 3));
}

void AxisPositionDisplay::updateDynamics(double velocity, double acceleration)
{
    velocityLabel->setText(QString::number(velocity, 'f', 2));
    accelerationLabel->setText(QString::number(acceleration, 'f', 2));
}

void AxisPositionDisplay::updateServoData(double torque, double followError, double temperature)
{
    torqueLabel->setText(QString::number(torque, 'f', 1) + "%");
    followErrorLabel->setText(QString::number(followError, 'f', 3));
    tempLabel->setText(QString::number(temperature, 'f', 1) + "°C");
}
