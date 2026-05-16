#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QStringList>
#include <QVector>
#include <QDateTime>

class CommandPanel : public QWidget {
    Q_OBJECT

public:
    explicit CommandPanel(QWidget *parent = nullptr);

signals:
    void commandRequested(const QString &cmd, const QVector<double> &args);

private:
    void setupUI();
    QVector<QDoubleSpinBox*> resolvePresetInputs(const QStringList &inputNames) const;

    void sendGenericCommand();
    void sendPreset(const QString &cmd, const QVector<QDoubleSpinBox*> &inputs);

    QLineEdit *cmdNameEdit_;
    QLineEdit *cmdArgsEdit_;
};
