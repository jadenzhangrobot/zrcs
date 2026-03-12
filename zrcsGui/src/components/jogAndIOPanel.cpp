#include "core/mainwindow_refactored.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

JogControlPanel::JogControlPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void JogControlPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 步长设置
    QGroupBox *stepGroup = new QGroupBox("步长设置", this);
    stepGroup->setStyleSheet("QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QHBoxLayout *stepLayout = new QHBoxLayout(stepGroup);
    
    stepSizeCombo = new QComboBox();
    stepSizeCombo->addItems({"连续", "10mm", "1mm", "0.1mm", "0.01mm"});
    stepSizeCombo->setStyleSheet("background-color: #2a2a2a; color: #00FF00; border: 1px solid #555;");
    stepLayout->addWidget(new QLabel("移动模式:"));
    stepLayout->addWidget(stepSizeCombo);
    
    connect(stepSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        double sizes[] = {0, 10, 1, 0.1, 0.01};
        emit stepSizeChanged(sizes[index]);
    });
    
    mainLayout->addWidget(stepGroup);
    
    // 倍率覆盖
    QGroupBox *overrideGroup = new QGroupBox("速度倍率", this);
    overrideGroup->setStyleSheet("QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QHBoxLayout *overrideLayout = new QHBoxLayout(overrideGroup);
    
    overrideSlider = new QSlider(Qt::Horizontal);
    overrideSlider->setRange(0, 100);
    overrideSlider->setValue(100);
    overrideSlider->setStyleSheet("QSlider::groove:horizontal { background: #444; height: 8px; } QSlider::handle:horizontal { background: #00FF00; width: 18px; margin: -5px 0; border-radius: 9px; }");
    
    overrideLabel = new QLabel("100%");
    overrideLabel->setStyleSheet("color: #00FF00; font-weight: bold; min-width: 40px;");
    
    connect(overrideSlider, &QSlider::valueChanged, this, [this](int value) {
        overrideLabel->setText(QString::number(value) + "%");
        emit overrideChanged(value);
    });
    
    overrideLayout->addWidget(overrideSlider);
    overrideLayout->addWidget(overrideLabel);
    mainLayout->addWidget(overrideGroup);
    
    // 点动控制
    QGroupBox *jogGroup = new QGroupBox("点动控制", this);
    jogGroup->setStyleSheet("QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QGridLayout *jogLayout = new QGridLayout(jogGroup);
    
    for (int i = 0; i < 5; ++i) {
        QPushButton *plusBtn = new QPushButton(QString("轴%1+").arg(i));
        QPushButton *minusBtn = new QPushButton(QString("轴%1-").arg(i));
        QPushButton *homeBtn = new QPushButton(QString("轴%1回零").arg(i));
        
        plusBtn->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; border-radius: 3px; padding: 8px; font-weight: bold;");
        minusBtn->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; border-radius: 3px; padding: 8px; font-weight: bold;");
        homeBtn->setStyleSheet("background-color: #5a5a2a; color: #FFD700; border: 1px solid #FFD700; border-radius: 3px; padding: 8px; font-weight: bold;");
        
        int axis = i;
        connect(plusBtn, &QPushButton::pressed, this, [this, axis]() { emit jogPressed(axis, 1); });
        connect(plusBtn, &QPushButton::released, this, [this, axis]() { emit jogReleased(axis); });
        connect(minusBtn, &QPushButton::pressed, this, [this, axis]() { emit jogPressed(axis, -1); });
        connect(minusBtn, &QPushButton::released, this, [this, axis]() { emit jogReleased(axis); });
        connect(homeBtn, &QPushButton::clicked, this, [this, axis]() { emit homeRequested(axis); });
        
        plusButtons.append(plusBtn);
        minusButtons.append(minusBtn);
        homeButtons.append(homeBtn);
        
        jogLayout->addWidget(plusBtn, i, 0);
        jogLayout->addWidget(minusBtn, i, 1);
        jogLayout->addWidget(homeBtn, i, 2);
    }
    
    QPushButton *homeAllBtn = new QPushButton("全轴回零");
    homeAllBtn->setStyleSheet("background-color: #5a2a2a; color: #FF6347; border: 1px solid #FF6347; border-radius: 3px; padding: 10px; font-weight: bold;");
    connect(homeAllBtn, &QPushButton::clicked, this, &JogControlPanel::homeAllRequested);
    jogLayout->addWidget(homeAllBtn, 5, 0, 1, 3);
    
    mainLayout->addWidget(jogGroup);
    mainLayout->addStretch();
    
    setStyleSheet("background-color: #1a1a1a;");
}

void JogControlPanel::setAxisCount(int)
{
    // 可根据需要调整轴数
}

IOPanel::IOPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void IOPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 输入监控
    QGroupBox *inputGroup = new QGroupBox("输入监控 (传感器/限位)", this);
    inputGroup->setStyleSheet("QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QGridLayout *inputLayout = new QGridLayout(inputGroup);
    
    QStringList inputNames = {"X限位+", "X限位-", "Y限位+", "Y限位-", "Z限位+", "Z限位-", "急停按钮", "使能开关"};
    for (int i = 0; i < inputNames.size(); ++i) {
        QLabel *led = new QLabel("●");
        led->setStyleSheet("color: #333333; font-size: 20pt;");
        QLabel *nameLabel = new QLabel(inputNames[i]);
        nameLabel->setStyleSheet("color: #CCCCCC;");
        
        inputLEDs.append(led);
        inputLayout->addWidget(led, i / 4, (i % 4) * 2);
        inputLayout->addWidget(nameLabel, i / 4, (i % 4) * 2 + 1);
    }
    
    mainLayout->addWidget(inputGroup);
    
    // 输出控制
    QGroupBox *outputGroup = new QGroupBox("输出控制 (执行器/继电器)", this);
    outputGroup->setStyleSheet("QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QGridLayout *outputLayout = new QGridLayout(outputGroup);
    
    QStringList outputNames = {"主轴启动", "冷却液", "气缸1", "气缸2", "继电器1", "继电器2"};
    for (int i = 0; i < outputNames.size(); ++i) {
        QPushButton *btn = new QPushButton(outputNames[i]);
        btn->setCheckable(true);
        btn->setStyleSheet("background-color: #2a2a2a; color: #CCCCCC; border: 1px solid #555; border-radius: 3px; padding: 8px;");
        
        int index = i;
        connect(btn, &QPushButton::toggled, this, [this, index](bool checked) {
            emit outputToggled(index, checked);
        });
        
        outputButtons.append(btn);
        outputLayout->addWidget(btn, i / 3, i % 3);
    }
    
    mainLayout->addWidget(outputGroup);
    mainLayout->addStretch();
    
    setStyleSheet("background-color: #1a1a1a;");
}

void IOPanel::updateInputState(int index, bool state)
{
    if (index >= 0 && index < inputLEDs.size()) {
        inputLEDs[index]->setStyleSheet(state ? "color: #00FF00; font-size: 20pt;" : "color: #333333; font-size: 20pt;");
    }
}

void IOPanel::updateOutputState(int index, bool state)
{
    if (index >= 0 && index < outputButtons.size()) {
        outputButtons[index]->setChecked(state);
    }
}
