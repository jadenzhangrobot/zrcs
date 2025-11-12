#include "mainwindow.h"
#include "ui_zrcsgui.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),nrtProcess(new NRTProcess("rtMotion")), moveTimer(new QTimer(this))
{
    ui->setupUi(this);
    
    // 初始化定时器
    moveTimer->setSingleShot(false);  // 重复触发
    moveTimer->setInterval(1000);     // 1秒间隔
    connect(moveTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    
    setupConnections();
    
    // 初始化NRT进程（创建共享内存）
    if (!nrtProcess->initialize()) 
    {
        throw std::runtime_error("NRT进程初始化失败");
        
    }
      Command cmd_;
      std::string cmd="ContinuousJog";
      strncpy(cmd_.cmd, cmd.c_str(), sizeof(cmd_.cmd) - 1);
      cmd_.cmd[sizeof(cmd_.cmd) - 1] = '\0';  // Ensure null termination
      nrtProcess->shared_block_->commandQueue.push(cmd_);

    // 设置窗口标题
    setWindowTitle("ZRCS控制系统");
    
    // 显示欢迎信息
    showStatusMessage("系统已就绪");
}

MainWindow::~MainWindow()
{
    delete nrtProcess;
    delete ui;
}

void MainWindow::setupConnections()
{
    // 连接控制按钮信号槽
    // +X按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton, &QPushButton::pressed, this, &MainWindow::onPlusXPressed);
    connect(ui->pushButton, &QPushButton::released, this, &MainWindow::onPlusXReleased);
    // -X按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_4, &QPushButton::pressed, this, &MainWindow::onMinusXPressed);
    connect(ui->pushButton_4, &QPushButton::released, this, &MainWindow::onMinusXReleased);


    // +Y按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_3, &QPushButton::pressed, this, &MainWindow::onPlusYPressed);
    connect(ui->pushButton_3, &QPushButton::released, this, &MainWindow::onPlusYReleased);
    // -Y按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_2, &QPushButton::pressed, this, &MainWindow::onMinusYPressed);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::onMinusYReleased);
    // +Z按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_5, &QPushButton::pressed, this, &MainWindow::onPlusZPressed);
    connect(ui->pushButton_5, &QPushButton::released, this, &MainWindow::onPlusZReleased);
    // -Z按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_11, &QPushButton::pressed, this, &MainWindow::onMinusZPressed);
    connect(ui->pushButton_11, &QPushButton::released, this, &MainWindow::onMinusZReleased);
    // +α按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_13, &QPushButton::pressed, this, &MainWindow::onPlusAlphaPressed);
    connect(ui->pushButton_13, &QPushButton::released, this, &MainWindow::onPlusAlphaReleased);
    // -α按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_15, &QPushButton::pressed, this, &MainWindow::onMinusAlphaPressed);
    connect(ui->pushButton_15, &QPushButton::released, this, &MainWindow::onMinusAlphaReleased);
    // +β按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_16, &QPushButton::pressed, this, &MainWindow::onPlusBetaPressed);
    connect(ui->pushButton_16, &QPushButton::released, this, &MainWindow::onPlusBetaReleased);
    // -β按钮使用pressed和released信号实现长按功能
    connect(ui->pushButton_14, &QPushButton::pressed, this, &MainWindow::onMinusBetaPressed);
    connect(ui->pushButton_14, &QPushButton::released, this, &MainWindow::onMinusBetaReleased);
    
    // 连接回零按钮信号槽
    connect(ui->pushButton_17, &QPushButton::clicked, this, &MainWindow::onXAxisHomeClicked);
    connect(ui->pushButton_18, &QPushButton::clicked, this, &MainWindow::onYAxisHomeClicked);
    connect(ui->pushButton_19, &QPushButton::clicked, this, &MainWindow::onZAxisHomeClicked);
    connect(ui->pushButton_20, &QPushButton::clicked, this, &MainWindow::onAlphaAxisHomeClicked);
    connect(ui->pushButton_21, &QPushButton::clicked, this, &MainWindow::onBetaAxisHomeClicked);
    connect(ui->pushButton_22, &QPushButton::clicked, this, &MainWindow::onAllAxisHomeClicked);
}
void MainWindow::onPlusXClicked()
{
    qDebug() << "X轴正向移动";
    showStatusMessage("X轴正向移动");
    // TODO: 在这里添加实际的X轴正向移动控制代码
   // SingleAxisMotion sam;
//   //  sam.axisId=0;
//     sam.motion=true;
//     sam.direction=true;
//     nrt_process->shared_block_->Multiplied.store(100);    
//     nrt_process->shared_block_->manualPosition.store(sam);
}
// 控制按钮槽函数实现
void MainWindow::onPlusXPressed()
{
    qDebug() << "X轴正向移动开始";
    showStatusMessage("X轴正向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onPlusXClicked(); };
    
    // 立即发送第一个移动命令
    onPlusXClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onPlusXReleased()
{
    qDebug() << "X轴正向移动停止";
    showStatusMessage("X轴正向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // SingleAxisMotion sam;
    // sam.axisId=0;
    // sam.motion=false;
    // sam.direction=true;
    // nrt_process->shared_block_->Multiplied.store(100);
    // nrt_process->shared_block_->manualPosition.store(sam);
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}
void MainWindow::onMinusXClicked()
{
    qDebug() << "X轴负向移动";
    showStatusMessage("X轴负向移动");
    
//     SingleAxisMotion sam;
//     sam.axisId=0;
//     sam.direction=false;
//     sam.motion=true;
//    nrt_process->shared_block_->Multiplied.store(100);
//     nrt_process->shared_block_->manualPosition.store(sam);
    // TODO: 在这里添加实际的X轴负向移动控制代码
}


void MainWindow::onMinusXPressed()
{
    qDebug() << "X轴负向移动开始";
    showStatusMessage("X轴负向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onMinusXClicked(); };
    
    // 立即发送第一个移动命令
    onMinusXClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onMinusXReleased()
{
    qDebug() << "X轴负向移动停止";
    showStatusMessage("X轴负向移动停止");
    //  SingleAxisMotion sam;
    // sam.axisId=0;
    // sam.motion=false;
    // sam.direction=false;
    // nrt_process->shared_block_->Multiplied.store(100);
    // nrt_process->shared_block_->manualPosition.store(sam);
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onPlusYPressed()
{
    qDebug() << "Y轴正向移动开始";
    showStatusMessage("Y轴正向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onPlusYClicked(); };
    
    // 立即发送第一个移动命令
    onPlusYClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onPlusYReleased()
{
    qDebug() << "Y轴正向移动停止";
    showStatusMessage("Y轴正向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onMinusYPressed()
{
    qDebug() << "Y轴负向移动开始";
    showStatusMessage("Y轴负向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onMinusYClicked(); };
    
    // 立即发送第一个移动命令
    onMinusYClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onMinusYReleased()
{
    qDebug() << "Y轴负向移动停止";
    showStatusMessage("Y轴负向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onPlusZPressed()
{
    qDebug() << "Z轴正向移动开始";
    showStatusMessage("Z轴正向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onPlusZClicked(); };
    
    // 立即发送第一个移动命令
    onPlusZClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onPlusZReleased()
{
    qDebug() << "Z轴正向移动停止";
    showStatusMessage("Z轴正向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onMinusZPressed()
{
    qDebug() << "Z轴负向移动开始";
    showStatusMessage("Z轴负向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onMinusZClicked(); };
    
    // 立即发送第一个移动命令
    onMinusZClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onMinusZReleased()
{
    qDebug() << "Z轴负向移动停止";
    showStatusMessage("Z轴负向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onPlusAlphaPressed()
{
    qDebug() << "α轴正向移动开始";
    showStatusMessage("α轴正向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onPlusAlphaClicked(); };
    
    // 立即发送第一个移动命令
    onPlusAlphaClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onPlusAlphaReleased()
{
    qDebug() << "α轴正向移动停止";
    showStatusMessage("α轴正向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onMinusAlphaPressed()
{
    qDebug() << "α轴负向移动开始";
    showStatusMessage("α轴负向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onMinusAlphaClicked(); };
    
    // 立即发送第一个移动命令
    onMinusAlphaClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onMinusAlphaReleased()
{
    qDebug() << "α轴负向移动停止";
    showStatusMessage("α轴负向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onPlusBetaPressed()
{
    qDebug() << "β轴正向移动开始";
    showStatusMessage("β轴正向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onPlusBetaClicked(); };
    
    // 立即发送第一个移动命令
    onPlusBetaClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onPlusBetaReleased()
{
    qDebug() << "β轴正向移动停止";
    showStatusMessage("β轴正向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onMinusBetaPressed()
{
    qDebug() << "β轴负向移动开始";
    showStatusMessage("β轴负向移动开始");
    
    // 设置当前移动函数
    currentMoveFunction = [this]() { onMinusBetaClicked(); };
    
    // 立即发送第一个移动命令
    onMinusBetaClicked();
    
    // 启动定时器，每1秒发送一次命令
    moveTimer->start();
}

void MainWindow::onMinusBetaReleased()
{
    qDebug() << "β轴负向移动停止";
    showStatusMessage("β轴负向移动停止");
    
    // 停止定时器
    moveTimer->stop();
    
    // 发送停止命令
    // TODO: 在这里添加实际的停止命令
}

void MainWindow::onTimerTimeout()
{
    // 定时器触发时发送移动命令
    if (currentMoveFunction) {
        currentMoveFunction();
    }
}

void MainWindow::onPlusYClicked()
{
    qDebug() << "Y轴正向移动";
    showStatusMessage("Y轴正向移动");
    // TODO: 在这里添加实际的Y轴正向移动控制代码
}

void MainWindow::onMinusYClicked()
{
    qDebug() << "Y轴负向移动";
    showStatusMessage("Y轴负向移动");
    // TODO: 在这里添加实际的Y轴负向移动控制代码
}

void MainWindow::onPlusZClicked()
{
    qDebug() << "Z轴正向移动";
    showStatusMessage("Z轴正向移动");
    // TODO: 在这里添加实际的Z轴正向移动控制代码
}

void MainWindow::onMinusZClicked()
{
    qDebug() << "Z轴负向移动";
    showStatusMessage("Z轴负向移动");
    // TODO: 在这里添加实际的Z轴负向移动控制代码
}

void MainWindow::onPlusAlphaClicked()
{
    qDebug() << "α轴正向旋转";
    showStatusMessage("α轴正向旋转");
    // TODO: 在这里添加实际的α轴正向旋转控制代码
}

void MainWindow::onMinusAlphaClicked()
{
    qDebug() << "α轴负向旋转";
    showStatusMessage("α轴负向旋转");
    // TODO: 在这里添加实际的α轴负向旋转控制代码
}

void MainWindow::onPlusBetaClicked()
{
    qDebug() << "β轴正向旋转";
    showStatusMessage("β轴正向旋转");
    // TODO: 在这里添加实际的β轴正向旋转控制代码
}

void MainWindow::onMinusBetaClicked()
{
    qDebug() << "β轴负向旋转";
    showStatusMessage("β轴负向旋转");
    // TODO: 在这里添加实际的β轴负向旋转控制代码
}

// 回零按钮槽函数实现
void MainWindow::onXAxisHomeClicked()
{
    qDebug() << "X轴回零";
    showStatusMessage("X轴回零中...");
    QMessageBox::information(this, "回零操作", "X轴回零操作已启动");
    // TODO: 在这里添加实际的X轴回零控制代码
}

void MainWindow::onYAxisHomeClicked()
{
    qDebug() << "Y轴回零";
    showStatusMessage("Y轴回零中...");
    QMessageBox::information(this, "回零操作", "Y轴回零操作已启动");
    // TODO: 在这里添加实际的Y轴回零控制代码
}

void MainWindow::onZAxisHomeClicked()
{
    qDebug() << "Z轴回零";
    showStatusMessage("Z轴回零中...");
    QMessageBox::information(this, "回零操作", "Z轴回零操作已启动");
    // TODO: 在这里添加实际的Z轴回零控制代码
}

void MainWindow::onAlphaAxisHomeClicked()
{
    qDebug() << "α轴回零";
    showStatusMessage("α轴回零中...");
    QMessageBox::information(this, "回零操作", "α轴回零操作已启动");
    // TODO: 在这里添加实际的α轴回零控制代码
}

void MainWindow::onBetaAxisHomeClicked()
{
    qDebug() << "β轴回零";
    showStatusMessage("β轴回零中...");
    QMessageBox::information(this, "回零操作", "β轴回零操作已启动");
    // TODO: 在这里添加实际的β轴回零控制代码
}

void MainWindow::onAllAxisHomeClicked()
{
    qDebug() << "所有轴回零";
    showStatusMessage("所有轴回零中...");
    QMessageBox::information(this, "回零操作", "所有轴回零操作已启动");
    // TODO: 在这里添加实际的所有轴回零控制代码
}

void MainWindow::showStatusMessage(const QString &message)
{
    if (ui->statusbar) {
        ui->statusbar->showMessage(message, 3000); // 显示3秒
    }
}