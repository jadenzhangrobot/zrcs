#include "command/CommandPanel.h"
#include "ui_command_panel.h"

#include <QStringList>

namespace {

constexpr double kSpinBoxMinimum = -999999.0;
constexpr double kSpinBoxMaximum = 999999.0;
constexpr int kSpinBoxDecimals = 3;

}  // namespace

CommandPanel::CommandPanel(QWidget *parent)
    : QWidget(parent),
      cmdNameEdit_(nullptr),
      cmdArgsEdit_(nullptr)
{
    setupUI();
}

void CommandPanel::setupUI()
{
    Ui::CommandPanelUi ui;
    ui.setupUi(this);

    cmdNameEdit_ = findChild<QLineEdit*>("cmdNameEdit");
    cmdArgsEdit_ = findChild<QLineEdit*>("cmdArgsEdit");
    auto *btnSendGeneric = findChild<QPushButton*>("btnSendGeneric");
    auto *logGroup = findChild<QGroupBox*>("logGroup");

    if (!cmdNameEdit_ || !cmdArgsEdit_ || !btnSendGeneric) {
        return;
    }

    if (logGroup) {
        logGroup->hide();
    }

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

        const QStringList inputNames = button->property("presetInputs")
                                           .toString()
                                           .split(',', Qt::SkipEmptyParts);

        connect(button, &QPushButton::clicked, this,
                [this, commandName, inputNames]() {
                    sendPreset(commandName, resolvePresetInputs(inputNames));
                });
    }

    connect(btnSendGeneric, &QPushButton::clicked,
            this, &CommandPanel::sendGenericCommand);
    connect(cmdArgsEdit_, &QLineEdit::returnPressed,
            this, &CommandPanel::sendGenericCommand);
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

    emit commandRequested(cmd, args);
}
