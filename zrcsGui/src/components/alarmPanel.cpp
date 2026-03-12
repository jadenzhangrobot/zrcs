#include "core/mainwindow_refactored.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>

AlarmPanel::AlarmPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 实时报警横幅
    QLabel *alarmBannerLabel = new QLabel("实时报警");
    alarmBannerLabel->setStyleSheet("color: #FF6347; font-size: 14pt; font-weight: bold;");
    mainLayout->addWidget(alarmBannerLabel);
    
    // 报警表格
    alarmTable = new QTableWidget();
    alarmTable->setColumnCount(3);
    alarmTable->setHorizontalHeaderLabels({"时间戳", "报警类型", "详情"});
    alarmTable->horizontalHeader()->setStretchLastSection(true);
    alarmTable->setStyleSheet(
        "QTableWidget { background-color: #1a1a1a; color: #CCCCCC; border: 1px solid #444; }"
        "QHeaderView::section { background-color: #2a2a2a; color: #FFD700; padding: 5px; border: 1px solid #444; }"
        "QTableWidget::item { padding: 5px; border-bottom: 1px solid #333; }"
    );
    alarmTable->setMaximumHeight(150);
    mainLayout->addWidget(alarmTable);
    
    // 历史日志
    QLabel *logLabel = new QLabel("操作日志");
    logLabel->setStyleSheet("color: #FFD700; font-size: 12pt; font-weight: bold;");
    mainLayout->addWidget(logLabel);
    
    logDisplay = new QTextEdit();
    logDisplay->setReadOnly(true);
    logDisplay->setStyleSheet(
        "QTextEdit { background-color: #0a0a0a; color: #00FF00; border: 1px solid #444; font-family: monospace; font-size: 9pt; }"
    );
    mainLayout->addWidget(logDisplay);
    
    // 控制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *clearAlarmsBtn = new QPushButton("清除报警");
    clearAlarmsBtn->setStyleSheet("background-color: #5a2a2a; color: #FF6347; border: 1px solid #FF6347; border-radius: 3px; padding: 8px; font-weight: bold;");
    connect(clearAlarmsBtn, &QPushButton::clicked, this, &AlarmPanel::clearAlarms);
    buttonLayout->addWidget(clearAlarmsBtn);
    
    QPushButton *exportLogBtn = new QPushButton("导出日志");
    exportLogBtn->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; border-radius: 3px; padding: 8px; font-weight: bold;");
    buttonLayout->addWidget(exportLogBtn);
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    setStyleSheet("background-color: #1a1a1a;");
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
