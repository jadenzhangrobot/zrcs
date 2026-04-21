#include "core/MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include "ui_alarm_panel.h"

AlarmPanel::AlarmPanel(QWidget *parent)
    : QWidget(parent)
{
    Ui::AlarmPanelUi ui;
    ui.setupUi(this);

    alarmTable = findChild<QTableWidget*>("alarmTable");
    logDisplay = findChild<QTextEdit*>("logDisplay");
    QPushButton *clearAlarmsBtn = findChild<QPushButton*>("clearAlarmsBtn");
    QPushButton *exportLogBtn = findChild<QPushButton*>("exportLogBtn");

    if (alarmTable) {
        alarmTable->setColumnCount(3);
        alarmTable->setHorizontalHeaderLabels({"时间戳", "报警类型", "详情"});
        alarmTable->horizontalHeader()->setStretchLastSection(true);
    }
    if (logDisplay) {
        logDisplay->setReadOnly(true);
    }
    if (clearAlarmsBtn) {
        clearAlarmsBtn->setProperty("kind", "danger");
        connect(clearAlarmsBtn, &QPushButton::clicked, this, &AlarmPanel::clearAlarms);
    }
    if (exportLogBtn) {
        exportLogBtn->setProperty("kind", "secondary");
    }
}

void AlarmPanel::addAlarm(const QString &message, const QString &timestamp)
{
    int row = alarmTable->rowCount();
    alarmTable->insertRow(row);
    
    QTableWidgetItem *timeItem = new QTableWidgetItem(timestamp);
    timeItem->setForeground(QColor(200, 200, 200));
    
    QTableWidgetItem *typeItem = new QTableWidgetItem("错误");
    typeItem->setForeground(QColor(255, 100, 100));
    
    QTableWidgetItem *msgItem = new QTableWidgetItem(message);
    msgItem->setForeground(QColor(255, 150, 150));
    
    alarmTable->setItem(row, 0, timeItem);
    alarmTable->setItem(row, 1, typeItem);
    alarmTable->setItem(row, 2, msgItem);
    
    // 添加到日志
    logDisplay->append(QString("[%1] %2").arg(timestamp, message));
}

void AlarmPanel::addLogEntry(quint32 level, const QString &source, const QString &message, const QString &timestamp)
{
    if (!logDisplay || !alarmTable) return;

    QString typeText = "信息";
    QColor typeColor(180, 180, 180);
    QColor msgColor(210, 210, 210);

    if (level == 1) {
        typeText = "警告";
        typeColor = QColor(255, 196, 64);
        msgColor = QColor(255, 220, 140);
    } else if (level >= 2) {
        typeText = "错误";
        typeColor = QColor(255, 100, 100);
        msgColor = QColor(255, 150, 150);
    }

    const QString fullMessage = QString("[RT] [%1] %2").arg(source, message);
    logDisplay->append(QString("[%1] [%2] %3").arg(timestamp, typeText, fullMessage));

    if (level < 1) {
        return;
    }

    int row = alarmTable->rowCount();
    alarmTable->insertRow(row);

    QTableWidgetItem *timeItem = new QTableWidgetItem(timestamp);
    timeItem->setForeground(QColor(200, 200, 200));

    QTableWidgetItem *typeItem = new QTableWidgetItem(typeText);
    typeItem->setForeground(typeColor);

    QTableWidgetItem *msgItem = new QTableWidgetItem(fullMessage);
    msgItem->setForeground(msgColor);

    alarmTable->setItem(row, 0, timeItem);
    alarmTable->setItem(row, 1, typeItem);
    alarmTable->setItem(row, 2, msgItem);
}

void AlarmPanel::clearAlarms()
{
    alarmTable->setRowCount(0);
}
