#include "core/MainWindow.h"
#include <QFile>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include "ui_alarm_panel.h"

AlarmPanel::AlarmPanel(QWidget *parent)
    : QWidget(parent)
{
    Ui::AlarmPanelUi ui;
    ui.setupUi(this);

    alarmTable = findChild<QTableWidget*>("alarmTable");
    logDisplay = findChild<QTextEdit*>("logDisplay");
    QPushButton *exportLogBtn = findChild<QPushButton*>("exportLogBtn");

    if (alarmTable) {
        alarmTable->setColumnCount(3);
        alarmTable->setHorizontalHeaderLabels({"时间戳", "报警类型", "详情"});
        alarmTable->horizontalHeader()->setStretchLastSection(true);
    }
    if (logDisplay) {
        logDisplay->setReadOnly(true);
    }
    if (exportLogBtn) {
        exportLogBtn->setProperty("kind", "secondary");
        connect(exportLogBtn, &QPushButton::clicked, this, [this]() {
            const QString filePath = QFileDialog::getSaveFileName(
                this,
                QStringLiteral("导出日志"),
                QStringLiteral("alarm_log.txt"),
                QStringLiteral("Text Files (*.txt);;All Files (*)"));
            if (filePath.isEmpty()) {
                return;
            }

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QMessageBox::warning(this,
                                     QStringLiteral("导出失败"),
                                     QStringLiteral("无法写入日志文件。"));
                return;
            }

            QString content;
            content += QStringLiteral("[实时报警]\n");
            if (alarmTable && alarmTable->rowCount() > 0) {
                for (int row = 0; row < alarmTable->rowCount(); ++row) {
                    QStringList columns;
                    columns.reserve(alarmTable->columnCount());
                    for (int column = 0; column < alarmTable->columnCount(); ++column) {
                        auto *item = alarmTable->item(row, column);
                        columns.append(item ? item->text() : QString());
                    }
                    content += columns.join(QStringLiteral(" | "));
                    content += QLatin1Char('\n');
                }
            } else {
                content += QStringLiteral("无报警记录\n");
            }

            content += QStringLiteral("\n[操作日志]\n");
            if (logDisplay && !logDisplay->toPlainText().isEmpty()) {
                content += logDisplay->toPlainText();
                if (!content.endsWith(QLatin1Char('\n'))) {
                    content += QLatin1Char('\n');
                }
            } else {
                content += QStringLiteral("无操作日志\n");
            }

            file.write(content.toUtf8());
            file.close();

            QMessageBox::information(this,
                                     QStringLiteral("导出成功"),
                                     QStringLiteral("日志已导出到:\n%1").arg(filePath));
        });
    }
}

void AlarmPanel::addAlarm(const QString &message, const QString &timestamp, const QString &type)
{
    int row = alarmTable->rowCount();
    alarmTable->insertRow(row);
    
    QTableWidgetItem *timeItem = new QTableWidgetItem(timestamp);
    timeItem->setForeground(QColor(200, 200, 200));
    
    QTableWidgetItem *typeItem = new QTableWidgetItem(type);
    const bool isWarning = type.contains(QStringLiteral("警告"));
    typeItem->setForeground(isWarning ? QColor(255, 190, 80) : QColor(255, 100, 100));
    
    QTableWidgetItem *msgItem = new QTableWidgetItem(message);
    msgItem->setForeground(isWarning ? QColor(255, 210, 120) : QColor(255, 150, 150));
    
    alarmTable->setItem(row, 0, timeItem);
    alarmTable->setItem(row, 1, typeItem);
    alarmTable->setItem(row, 2, msgItem);
    
    alarmTable->scrollToBottom();
}

void AlarmPanel::appendOperationLog(const QString &message)
{
    if (logDisplay) {
        logDisplay->append(message);
    }
}
