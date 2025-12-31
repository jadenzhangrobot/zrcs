#pragma once
#include <cstring>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "common/sharedMemory/nrt_process.h"
#include "ui_zrcsgui.h"

class ManualControl
{
    Ui::MainWindow *ui;
    NRTProcess *nrtProcess;
public:
    ManualControl(Ui::MainWindow *ui, NRTProcess *nrtProcess)
    {
        this->ui = ui;
        this->nrtProcess = nrtProcess;
        
        // 确保scrollAreaWidgetContents有布局
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
        if (!layout) {
            layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
            ui->scrollAreaWidgetContents->setLayout(layout);
        }
        for (int i = 0; i < nrtProcess->shared_block_->axisCount.load(); i++)
        {
            QWidget *axisWidget = new QWidget(ui->scrollAreaWidgetContents);
            QHBoxLayout *hLayout = new QHBoxLayout(axisWidget);
            hLayout->setContentsMargins(0, 5, 0, 5);

            // 创建控件，模仿widget_4的结构
            QPushButton *btnMinus = new QPushButton("-", axisWidget);
            btnMinus->setFixedSize(41, 23);

            QLabel *label = new QLabel(QString("J%1").arg(i + 1), axisWidget);
            label->setAlignment(Qt::AlignCenter);

            QPushButton *btnPlus = new QPushButton("+", axisWidget);
            btnPlus->setFixedSize(41, 23);
          
            QLabel *labelposition = new QLabel(QString("当前位置"), axisWidget);
            labelposition->setAlignment(Qt::AlignCenter);


            QLineEdit *lineEdit = new QLineEdit(axisWidget);
            // 可以根据需要设置lineEdit的大小或策略

            hLayout->addWidget(btnMinus);
            hLayout->addWidget(label);
            hLayout->addWidget(btnPlus);
            hLayout->addWidget(labelposition);
            hLayout->addWidget(lineEdit);
            layout->addWidget(axisWidget);

            // 连接按钮信号到槽
            // 按钮按下时发送负速度指令
            QObject::connect(btnMinus, &QPushButton::pressed, btnMinus, [=]() {
                    singleAxisContinueMotion motion;
                    motion.axisId=i;
                    nrtProcess->shared_block_->Multiplied.store(ui->horizontalSlider->value(),std::memory_order_release);
                    motion.motion=true;
                    motion.direction=false;
                    nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
            });
            // 按钮松开时发送停止命令
            QObject::connect(btnMinus, &QPushButton::released, btnMinus, [=]() {
                    nrtProcess->shared_block_->Multiplied.store(ui->horizontalSlider->value(),std::memory_order_release);
                    singleAxisContinueMotion motion;
                    motion.axisId=i;
                    motion.motion=false;
                    nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
            });


            // 按钮按下时发送正速度指令
            QObject::connect(btnPlus, &QPushButton::pressed, btnPlus, [=]() {
                    nrtProcess->shared_block_->Multiplied.store(ui->horizontalSlider->value(),std::memory_order_release);
                    singleAxisContinueMotion motion;
                    motion.axisId=i;
                    motion.motion=true;
                    motion.direction=true;
                    nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
            });

            // 按钮松开时发送停止命令
            QObject::connect(btnPlus, &QPushButton::released, btnPlus, [=]() {
                    nrtProcess->shared_block_->Multiplied.store(ui->horizontalSlider->value(),std::memory_order_release);
                    singleAxisContinueMotion motion;
                    motion.axisId=i;
                    motion.motion=false;
                    nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
            });


        }
        // 添加弹簧，使控件靠上对齐
        layout->addStretch();
    }
    ~ManualControl()
    {
        
    }
};
