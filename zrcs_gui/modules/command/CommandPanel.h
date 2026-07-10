#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QStringList>
#include <QVector>

class CommandPanel : public QWidget {
    Q_OBJECT

public:
    explicit CommandPanel(QWidget *parent = nullptr);

signals:
    void commandRequested(const QString &cmd, const QVector<double> &args);

private:
    void setupUI();
    QVector<QDoubleSpinBox*> resolvePresetInputs(const QStringList &inputNames) const;
    void sendPreset(const QString &cmd, const QVector<QDoubleSpinBox*> &inputs);
};
