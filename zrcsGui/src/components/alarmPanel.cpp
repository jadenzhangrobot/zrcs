#include "core/mainwindow_refactored.h"
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

void AlarmPanel::clearAlarms()
{
    alarmTable->setRowCount(0);
}
