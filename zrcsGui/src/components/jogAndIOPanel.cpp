#include "core/mainwindow_refactored.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStyle>
#include "ui_jog_control_panel.h"
#include "ui_io_panel.h"

JogControlPanel::JogControlPanel(QWidget *parent)
    : QWidget(parent), axisCount(5), axisPageSize(5)
{
    setupUI();
    setAxisCount(axisCount);
}

void JogControlPanel::setupUI()
{
    Ui::JogControlPanelUi ui;
    ui.setupUi(this);

    stepSizeCombo = findChild<QComboBox*>("stepSizeCombo");
    axisGroupCombo = findChild<QComboBox*>("axisGroupCombo");
    overrideSlider = findChild<QSlider*>("overrideSlider");
    overrideLabel = findChild<QLabel*>("overrideLabel");
    QGridLayout *jogLayout = findChild<QGridLayout*>("jogGridLayout");
    if (!stepSizeCombo || !axisGroupCombo || !overrideSlider || !overrideLabel || !jogLayout) {
        return;
    }

    stepSizeCombo->clear();
    stepSizeCombo->addItems({"连续", "10mm", "1mm", "0.1mm", "0.01mm"});

    connect(stepSizeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        double sizes[] = {0, 10, 1, 0.1, 0.01};
        emit stepSizeChanged(sizes[index]);
    });
    connect(axisGroupCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshAxisButtons();
    });
    overrideSlider->setRange(0, 100);
    overrideSlider->setValue(100);
    overrideLabel->setText("100%");
    connect(overrideSlider, &QSlider::valueChanged, this, [this](int value) {
        overrideLabel->setText(QString::number(value) + "%");
        emit overrideChanged(value);
    });
    
    QStringList plusNames = {"btnPlus1", "btnPlus2", "btnPlus3", "btnPlus4", "btnPlus5"};
    QStringList minusNames = {"btnMinus1", "btnMinus2", "btnMinus3", "btnMinus4", "btnMinus5"};
    QStringList homeNames = {"btnHome1", "btnHome2", "btnHome3", "btnHome4", "btnHome5"};
    QStringList originNames = {"btnOrigin1", "btnOrigin2", "btnOrigin3", "btnOrigin4", "btnOrigin5"};
    QStringList posNames = {"lblPos1", "lblPos2", "lblPos3", "lblPos4", "lblPos5"};

    for (int i = 0; i < axisPageSize; ++i) {
        QPushButton *plusBtn = findChild<QPushButton*>(plusNames[i]);
        QPushButton *minusBtn = findChild<QPushButton*>(minusNames[i]);
        QPushButton *homeBtn = findChild<QPushButton*>(homeNames[i]);
        QPushButton *originBtn = findChild<QPushButton*>(originNames[i]);
        QLabel *posLabel = findChild<QLabel*>(posNames[i]);
        if (!plusBtn || !minusBtn || !homeBtn || !originBtn || !posLabel) {
            continue;
        }
        homeBtn->setProperty("kind", "secondary");
        originBtn->setProperty("kind", "secondary");
        plusBtn->setProperty("compact", true);
        minusBtn->setProperty("compact", true);
        posLabel->setObjectName("axisPosDisplay");
        connect(plusBtn, &QPushButton::pressed, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit jogPressed(axis, 1);
        });
        connect(plusBtn, &QPushButton::released, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit jogReleased(axis);
        });
        connect(minusBtn, &QPushButton::pressed, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit jogPressed(axis, -1);
        });
        connect(minusBtn, &QPushButton::released, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit jogReleased(axis);
        });
        connect(homeBtn, &QPushButton::clicked, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit homeRequested(axis);
        });
        connect(originBtn, &QPushButton::clicked, this, [this, i]() {
            int axis = axisGroupCombo->currentData().toInt() + i;
            if (axis < axisCount) emit setCurrentAsOriginRequested(axis);
        });
        plusButtons.append(plusBtn);
        minusButtons.append(minusBtn);
        homeButtons.append(homeBtn);
        axisPositionLabels.append(posLabel);
        originButtons.append(originBtn);
    }

    QPushButton *homeAllBtn = findChild<QPushButton*>("homeAllBtn");
    if (homeAllBtn) {
        homeAllBtn->setProperty("kind", "danger");
        connect(homeAllBtn, &QPushButton::clicked, this, &JogControlPanel::homeAllRequested);
    }
}

void JogControlPanel::setAxisCount(int count)
{
    axisCount = qMax(1, count);
    int groupCount = (axisCount + axisPageSize - 1) / axisPageSize;
    axisGroupCombo->blockSignals(true);
    axisGroupCombo->clear();
    for (int g = 0; g < groupCount; ++g) {
        int start = g * axisPageSize + 1;
        int end = qMin(axisCount, (g + 1) * axisPageSize);
        axisGroupCombo->addItem(QString("J%1-J%2").arg(start).arg(end), g * axisPageSize);
    }
    axisGroupCombo->setCurrentIndex(0);
    axisGroupCombo->blockSignals(false);
    refreshAxisButtons();
}

void JogControlPanel::refreshAxisButtons()
{
    int startAxis = axisGroupCombo ? axisGroupCombo->currentData().toInt() : 0;
    for (int i = 0; i < plusButtons.size(); ++i) {
        int axis = startAxis + i;
        bool enabled = axis < axisCount;
        plusButtons[i]->setVisible(enabled);
        minusButtons[i]->setVisible(enabled);
        homeButtons[i]->setVisible(enabled);
        axisPositionLabels[i]->setVisible(enabled);
        originButtons[i]->setVisible(enabled);
        if (enabled) {
            plusButtons[i]->setText(QString("J%1 +").arg(axis + 1));
            minusButtons[i]->setText(QString("J%1 -").arg(axis + 1));
            homeButtons[i]->setText(QString("J%1 回零").arg(axis + 1));
            originButtons[i]->setText(QString("J%1 设原点").arg(axis + 1));
        }
    }
}

void JogControlPanel::setAxisPosition(int axis, double position)
{
    int startAxis = axisGroupCombo ? axisGroupCombo->currentData().toInt() : 0;
    int localIndex = axis - startAxis;
    if (localIndex >= 0 && localIndex < axisPositionLabels.size()) {
        axisPositionLabels[localIndex]->setText(QString::number(position, 'f', 3));
    }
}

IOPanel::IOPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void IOPanel::setupUI()
{
    Ui::IOPanelUi ui;
    ui.setupUi(this);
    QGridLayout *inputLayout = findChild<QGridLayout*>("inputGridLayout");
    QGridLayout *outputLayout = findChild<QGridLayout*>("outputGridLayout");
    if (!inputLayout || !outputLayout) {
        return;
    }
    
    QStringList inputNames = {"X限位+", "X限位-", "Y限位+", "Y限位-", "Z限位+", "Z限位-", "急停按钮", "使能开关"};
    for (int i = 0; i < inputNames.size(); ++i) {
        QLabel *led = new QLabel("●");
        led->setObjectName("ioInputLed");
        led->setProperty("active", false);
        QLabel *nameLabel = new QLabel(inputNames[i]);
        
        inputLEDs.append(led);
        inputLayout->addWidget(led, i / 4, (i % 4) * 2);
        inputLayout->addWidget(nameLabel, i / 4, (i % 4) * 2 + 1);
    }

    QStringList outputNames = {"主轴启动", "冷却液", "气缸1", "气缸2", "继电器1", "继电器2"};
    for (int i = 0; i < outputNames.size(); ++i) {
        QPushButton *btn = new QPushButton(outputNames[i]);
        btn->setCheckable(true);
        
        int index = i;
        connect(btn, &QPushButton::toggled, this, [this, index](bool checked) {
            emit outputToggled(index, checked);
        });
        
        outputButtons.append(btn);
        outputLayout->addWidget(btn, i / 3, i % 3);
    }
}

void IOPanel::updateInputState(int index, bool state)
{
    if (index >= 0 && index < inputLEDs.size()) {
        inputLEDs[index]->setProperty("active", state);
        inputLEDs[index]->style()->unpolish(inputLEDs[index]);
        inputLEDs[index]->style()->polish(inputLEDs[index]);
    }
}

void IOPanel::updateOutputState(int index, bool state)
{
    if (index >= 0 && index < outputButtons.size()) {
        outputButtons[index]->setChecked(state);
    }
}
