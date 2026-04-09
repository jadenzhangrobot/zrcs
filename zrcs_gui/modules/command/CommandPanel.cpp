#include "command/CommandPanel.h"
#include <QSplitter>
#include <QStringList>

CommandPanel::CommandPanel(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void CommandPanel::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 上半部分：命令区域（可滚动）
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollContent = new QWidget;
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(4, 4, 4, 4);
    scrollLayout->setSpacing(6);

    scrollLayout->addWidget(createGenericGroup());
    scrollLayout->addWidget(createSystemGroup());
    scrollLayout->addWidget(createSingleAxisGroup());
    scrollLayout->addWidget(createMultiAxisGroup());
    scrollLayout->addWidget(createIOGroup());
    scrollLayout->addWidget(createSchedulingGroup());
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);

    // 下半部分：命令日志
    auto *logGroup = new QGroupBox("命令日志");
    auto *logLayout = new QVBoxLayout(logGroup);
    logView_ = new QTextEdit;
    logView_->setReadOnly(true);
    logView_->setMaximumHeight(150);
    logView_->setPlaceholderText("命令发送记录将显示在这里...");
    logLayout->addWidget(logView_);

    // 使用 splitter 分割命令区和日志区
    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(scrollArea);
    splitter->addWidget(logGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

// ============================================================
// 通用命令区
// ============================================================

QGroupBox *CommandPanel::createGenericGroup()
{
    auto *group = new QGroupBox("通用命令");
    auto *layout = new QHBoxLayout(group);

    layout->addWidget(new QLabel("命令名:"));
    cmdNameEdit_ = new QLineEdit;
    cmdNameEdit_->setPlaceholderText("如 Enable, MoveJ...");
    cmdNameEdit_->setMinimumWidth(120);
    layout->addWidget(cmdNameEdit_);

    layout->addWidget(new QLabel("参数:"));
    cmdArgsEdit_ = new QLineEdit;
    cmdArgsEdit_->setPlaceholderText("逗号分隔, 如 0,100,50");
    cmdArgsEdit_->setMinimumWidth(200);
    layout->addWidget(cmdArgsEdit_);

    auto *btnSend = new QPushButton("发送");
    btnSend->setMinimumWidth(80);
    layout->addWidget(btnSend);

    connect(btnSend, &QPushButton::clicked, this, &CommandPanel::sendGenericCommand);
    connect(cmdArgsEdit_, &QLineEdit::returnPressed, this, &CommandPanel::sendGenericCommand);

    return group;
}

// ============================================================
// 预设命令分类
// ============================================================

QGroupBox *CommandPanel::createSystemGroup()
{
    auto *group = new QGroupBox("系统���令");
    auto *layout = new QVBoxLayout(group);
    addCommandRow(layout, "Enable",   {"轴号"}, {0});
    addCommandRow(layout, "Disable",  {"轴号"}, {0});
    addCommandRow(layout, "Reset",    {"轴号"}, {0});
    addCommandRow(layout, "SetZero",  {"轴号"}, {0});
    addButtonRow(layout, {"EmergStop", "Movehome", "GetFK", "GetJointPos"});
    return group;
}

QGroupBox *CommandPanel::createSingleAxisGroup()
{
    auto *group = new QGroupBox("单轴运动");
    auto *layout = new QVBoxLayout(group);
    addCommandRow(layout, "MoveAbs",  {"轴号", "位置", "速度", "加速度", "加加速度"}, {0, 0, 100, 500, 5000});
    addCommandRow(layout, "MoveRel",  {"轴号", "距离", "速度", "加速度", "加加速度"}, {0, 10, 100, 500, 5000});
    addCommandRow(layout, "JogabsJ",  {"轴号", "目标位置"}, {0, 0});
    addCommandRow(layout, "JogJ",     {"轴号", "目标位置"}, {0, 10});
    return group;
}

QGroupBox *CommandPanel::createMultiAxisGroup()
{
    auto *group = new QGroupBox("多轴运动");
    auto *layout = new QVBoxLayout(group);
    addCommandRow(layout, "MoveJ", {"X", "Y", "Z", "RX", "RY", "RZ", "速度"}, {0, 0, 0, 0, 0, 0, 50});
    addCommandRow(layout, "MoveL", {"X", "Y", "Z", "RX", "RY", "RZ", "速度"}, {0, 0, 0, 0, 0, 0, 50});
    addCommandRow(layout, "MoveC", {"ViaX", "ViaY", "ViaZ", "EndX", "EndY", "EndZ", "速度"}, {0, 0, 0, 0, 0, 0, 50});
    return group;
}

QGroupBox *CommandPanel::createIOGroup()
{
    auto *group = new QGroupBox("IO 命令");
    auto *layout = new QVBoxLayout(group);
    addCommandRow(layout, "SetDO",  {"模块", "位", "值"},     {0, 0, 1});
    addCommandRow(layout, "SetGO",  {"模块", "值"},           {0, 0});
    addCommandRow(layout, "SetAO",  {"模块", "通道", "值"},   {0, 0, 0});
    addCommandRow(layout, "PulseDO",{"模块", "位", "时长ms"}, {0, 0, 500});
    return group;
}

QGroupBox *CommandPanel::createSchedulingGroup()
{
    auto *group = new QGroupBox("调度控制");
    auto *layout = new QVBoxLayout(group);
    addButtonRow(layout, {"SYS_RUN", "SYS_STOP", "SYS_RESET", "SYS_ESTOP"});
    addCommandRow(layout, "SYS_JOG_START", {"轴号", "方向(1正/0负)"}, {0, 1});
    addButtonRow(layout, {"SYS_JOG_STOP"});
    addCommandRow(layout, "SYS_SET_MULTIPLIER", {"百分比"}, {100});
    return group;
}

// ============================================================
// 通用辅助方法
// ============================================================

void CommandPanel::addCommandRow(QVBoxLayout *parentLayout,
                                  const QString &cmdName,
                                  const QStringList &paramLabels,
                                  const QVector<double> &defaults)
{
    auto *row = new QHBoxLayout;
    row->setSpacing(4);

    auto *btn = new QPushButton(cmdName);
    btn->setFixedWidth(100);
    row->addWidget(btn);

    QVector<QDoubleSpinBox*> inputs;
    for (int i = 0; i < paramLabels.size(); ++i) {
        auto *label = new QLabel(paramLabels[i] + ":");
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(label);

        auto *spin = new QDoubleSpinBox;
        spin->setRange(-999999, 999999);
        spin->setDecimals(3);
        spin->setMinimumWidth(70);
        if (i < defaults.size()) {
            spin->setValue(defaults[i]);
        }
        row->addWidget(spin);
        inputs.append(spin);
    }

    row->addStretch();

    connect(btn, &QPushButton::clicked, this, [this, cmdName, inputs]() {
        sendPreset(cmdName, inputs);
    });

    parentLayout->addLayout(row);
}

void CommandPanel::addButtonRow(QVBoxLayout *parentLayout, const QStringList &cmdNames)
{
    auto *row = new QHBoxLayout;
    row->setSpacing(4);

    for (const auto &name : cmdNames) {
        auto *btn = new QPushButton(name);
        btn->setFixedWidth(100);
        connect(btn, &QPushButton::clicked, this, [this, name]() {
            sendPreset(name, {});
        });
        row->addWidget(btn);
    }
    row->addStretch();
    parentLayout->addLayout(row);
}

// ============================================================
// 发送逻辑
// ============================================================

void CommandPanel::sendGenericCommand()
{
    QString cmd = cmdNameEdit_->text().trimmed();
    if (cmd.isEmpty()) return;

    QVector<double> args;
    QString argsStr = cmdArgsEdit_->text().trimmed();
    if (!argsStr.isEmpty()) {
        for (const auto &token : argsStr.split(',', Qt::SkipEmptyParts)) {
            bool ok = false;
            double val = token.trimmed().toDouble(&ok);
            if (ok) args.append(val);
        }
    }

    QString logMsg = QString("[%1] %2(%3)")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(cmd)
        .arg(argsStr);
    appendLog(logMsg);

    emit commandRequested(cmd, args);
}

void CommandPanel::sendPreset(const QString &cmd, const QVector<QDoubleSpinBox*> &inputs)
{
    QVector<double> args;
    QStringList argStrs;
    for (auto *spin : inputs) {
        args.append(spin->value());
        argStrs.append(QString::number(spin->value(), 'f', 3));
    }

    QString logMsg = QString("[%1] %2(%3)")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(cmd)
        .arg(argStrs.join(", "));
    appendLog(logMsg);

    emit commandRequested(cmd, args);
}

void CommandPanel::appendLog(const QString &text)
{
    if (logView_) {
        logView_->append(text);
    }
}
