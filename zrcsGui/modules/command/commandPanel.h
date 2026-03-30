#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVector>
#include <QDateTime>
#include <QScrollArea>

class CommandPanel : public QWidget {
    Q_OBJECT

public:
    explicit CommandPanel(QWidget *parent = nullptr);

signals:
    void commandRequested(const QString &cmd, const QVector<double> &args);

public slots:
    void appendLog(const QString &text);

private:
    void setupUI();

    // 添加一行预设命令：按钮 + 参数输入框
    void addCommandRow(QVBoxLayout *layout,
                       const QString &cmdName,
                       const QStringList &paramLabels,
                       const QVector<double> &defaults = {});

    // 添加一行无参数的命令按钮（可多个并排）
    void addButtonRow(QVBoxLayout *layout, const QStringList &cmdNames);

    // 创建各分类
    QGroupBox *createGenericGroup();
    QGroupBox *createSystemGroup();
    QGroupBox *createSingleAxisGroup();
    QGroupBox *createMultiAxisGroup();
    QGroupBox *createIOGroup();
    QGroupBox *createSchedulingGroup();

    void sendGenericCommand();
    void sendPreset(const QString &cmd, const QVector<QDoubleSpinBox*> &inputs);

    QLineEdit *cmdNameEdit_;
    QLineEdit *cmdArgsEdit_;
    QTextEdit *logView_;
};
