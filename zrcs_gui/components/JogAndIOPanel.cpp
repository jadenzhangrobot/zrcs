#include "core/MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStyle>
#include "ui_jog_control_panel.h"

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
    axisScrollBar = findChild<QScrollBar*>("axisScrollBar");
    overrideSlider = findChild<QSlider*>("overrideSlider");
    overrideLabel = findChild<QLabel*>("overrideLabel");
    QGridLayout *jogLayout = findChild<QGridLayout*>("jogGridLayout");
    if (!stepSizeCombo || !axisGroupCombo || !axisScrollBar || !overrideSlider || !overrideLabel || !jogLayout) {
        return;
    }

    if (auto *axisGroupLabel = findChild<QLabel*>("axisGroupLabel")) {
        axisGroupLabel->setVisible(false);
    }
    axisGroupCombo->setVisible(false);
    axisScrollBar->hide();
    axisScrollBar->setSingleStep(1);
    axisScrollBar->setPageStep(1);

    stepSizeCombo->clear();
    // 界面按机床习惯显示 mm；发出的 stepSize 已是控制器单位 m（×0.001）。
    stepSizeCombo->addItems({"连续", "10mm", "1mm", "0.1mm", "0.01mm"});

    connect(stepSizeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        // UI mm → 控制器 m
        constexpr double kMmToM = 0.001;
        const double sizesMm[] = {0.0, 10.0, 1.0, 0.1, 0.01};
        const int i = qBound(0, index, 4);
        emit stepSizeChanged(sizesMm[i] * kMmToM);
    });
    connect(axisScrollBar, &QScrollBar::valueChanged, this, [this](int) {
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
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit jogPressed(axis, 1);
        });
        connect(plusBtn, &QPushButton::released, this, [this, i]() {
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit jogReleased(axis);
        });
        connect(minusBtn, &QPushButton::pressed, this, [this, i]() {
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit jogPressed(axis, -1);
        });
        connect(minusBtn, &QPushButton::released, this, [this, i]() {
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit jogReleased(axis);
        });
        connect(homeBtn, &QPushButton::clicked, this, [this, i]() {
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit homeRequested(axis);
        });
        connect(originBtn, &QPushButton::clicked, this, [this, i]() {
            int axis = (axisScrollBar ? axisScrollBar->value() : 0) + i;
            if (axis < axisCount) emit setCurrentAsOriginRequested(axis);
        });
        plusButtons.append(plusBtn);
        minusButtons.append(minusBtn);
        homeButtons.append(homeBtn);
        axisPositionLabels.append(posLabel);
        originButtons.append(originBtn);
    }

    servoStates_.resize(axisPageSize);

    QPushButton *homeAllBtn = findChild<QPushButton*>("homeAllBtn");
    if (homeAllBtn) {
        homeAllBtn->setProperty("kind", "danger");
        connect(homeAllBtn, &QPushButton::clicked, this, &JogControlPanel::homeAllRequested);
    }
}

void JogControlPanel::setAxisCount(int count)
{
    axisCount = qMax(1, count);
    axisPositions.resize(axisCount);
    if (axisScrollBar) {
        const int maxOffset = qMax(0, axisCount - plusButtons.size());
        axisScrollBar->blockSignals(true);
        axisScrollBar->setRange(0, maxOffset);
        axisScrollBar->setPageStep(1);
        axisScrollBar->setValue(qMin(axisScrollBar->value(), maxOffset));
        axisScrollBar->setVisible(maxOffset > 0);
        axisScrollBar->blockSignals(false);
    }
    refreshAxisButtons();
}

void JogControlPanel::refreshAxisButtons()
{
    const int startAxis = axisScrollBar ? axisScrollBar->value() : 0;
    for (int i = 0; i < plusButtons.size(); ++i) {
        const int axis = startAxis + i;
        const bool enabled = axis >= 0 && axis < axisCount;
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
            // 伺服使能指示: 绿点●=使能, 灰点○=失能
            const bool powered = axis < servoStates_.size() && servoStates_[axis] != 0;
            const QString dot = powered ? QStringLiteral("<font color='#4caf50'>●</font> ")
                                        : QStringLiteral("<font color='#666666'>○</font> ");
            axisPositionLabels[i]->setText(
                dot + QString::number(axisPositions.value(axis), 'f', 5));
        }
    }
}

void JogControlPanel::setAxisServoStates(const QVector<quint8> &states)
{
    servoStates_ = states;
    refreshAxisButtons();
}

void JogControlPanel::setAxisPosition(int axis, double position)
{
    if (axis < 0 || axis >= axisCount) {
        return;
    }

    if (axis >= axisPositions.size()) {
        axisPositions.resize(axis + 1);
    }
    axisPositions[axis] = position;

    const int startAxis = axisScrollBar ? axisScrollBar->value() : 0;
    const int localIndex = axis - startAxis;
    if (localIndex >= 0 && localIndex < axisPositionLabels.size()) {
        // position 为控制器单位（m 或 rad）
        axisPositionLabels[localIndex]->setText(QString::number(position, 'f', 5));
    }
}
