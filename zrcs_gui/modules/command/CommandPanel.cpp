#include "command/CommandPanel.h"
#include "ui_command_panel.h"

#include "config/CmdDefine.h"

#include <QSet>
#include <QStringList>

namespace {

constexpr double kSpinBoxMinimum = -999999.0;
constexpr double kSpinBoxMaximum = 999999.0;
constexpr int kSpinBoxDecimals = 3;

bool isSupportedPresetCommand(const QString &commandName)
{
    static const QSet<QString> kSystemCommands = {
        QStringLiteral("SYS_RUN"),
        QStringLiteral("SYS_STOP"),
        QStringLiteral("SYS_RESET"),
        QStringLiteral("SYS_ESTOP"),
        QStringLiteral("SYS_JOG_START"),
        QStringLiteral("SYS_JOG_STOP"),
        QStringLiteral("SYS_SET_MULTIPLIER"),
        QStringLiteral("SYS_SET_ORIGIN"),
        QStringLiteral("SYS_SET_AXIS_ORIGIN")
    };

    if (kSystemCommands.contains(commandName)) {
        return true;
    }

    const auto cmdId = zrcs::cmdNameToId(commandName.toStdString());
    return cmdId.has_value() && *cmdId != CmdId::INVALID;
}

}  // namespace

CommandPanel::CommandPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void CommandPanel::setupUI()
{
    Ui::CommandPanelUi ui;
    ui.setupUi(this);

    const auto spinBoxes = findChildren<QDoubleSpinBox*>();
    for (auto *spinBox : spinBoxes) {
        spinBox->setRange(kSpinBoxMinimum, kSpinBoxMaximum);
        spinBox->setDecimals(kSpinBoxDecimals);
    }

    const auto buttons = findChildren<QPushButton*>();
    for (auto *button : buttons) {
        const QString commandName = button->property("presetCommand").toString();
        if (commandName.isEmpty()) {
            continue;
        }

        if (!isSupportedPresetCommand(commandName)) {
            button->setEnabled(false);
            button->setToolTip(QStringLiteral("当前 RT/NRT 未注册该命令"));
            continue;
        }

        const QStringList inputNames = button->property("presetInputs")
                                           .toString()
                                           .split(',', Qt::SkipEmptyParts);

        connect(button, &QPushButton::clicked, this,
                [this, commandName, inputNames]() {
                    sendPreset(commandName, resolvePresetInputs(inputNames));
                });
    }
}

QVector<QDoubleSpinBox*> CommandPanel::resolvePresetInputs(const QStringList &inputNames) const
{
    QVector<QDoubleSpinBox*> inputs;
    inputs.reserve(inputNames.size());

    for (const QString &inputName : inputNames) {
        auto *spinBox = findChild<QDoubleSpinBox*>(inputName.trimmed());
        if (spinBox) {
            inputs.append(spinBox);
        }
    }

    return inputs;
}

void CommandPanel::sendPreset(const QString &cmd, const QVector<QDoubleSpinBox*> &inputs)
{
    QVector<double> args;
    for (auto *spin : inputs) {
        args.append(spin->value());
    }

    emit commandRequested(cmd, args);
}
