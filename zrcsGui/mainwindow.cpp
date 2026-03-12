#include "mainwindow.h"
#include "config/cmdArgs.h"
#include "sharedMemory/sharedData.h"
#include "ui_zrcsgui.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), nrtProcess(new NRTProcess("rtMotion")), 
      moveTimer(new QTimer(this)), useZMQ_(false)
{
    ui->setupUi(this);
    moveTimer->setSingleShot(false);
    moveTimer->setInterval(100);
    connect(moveTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    
    setupConnections();
    
    if (!nrtProcess->initialize()) {
        throw std::runtime_error("NRT进程初始化失败");  
    }
    manualControl = new ManualControl(ui, nrtProcess);

    // 初始化 ZMQ 客户端
    zmqClient = new ZMQClient("localhost", 5555, this);
    connect(zmqClient, &ZMQClient::connected, this, &MainWindow::onZMQConnected);
    connect(zmqClient, &ZMQClient::disconnected, this, &MainWindow::onZMQDisconnected);
    connect(zmqClient, &ZMQClient::commandSent, this, &MainWindow::onZMQCommandSent);
    connect(zmqClient, &ZMQClient::errorOccurred, this, &MainWindow::onZMQError);
    zmqClient->connectToServer();

    setWindowTitle("ZRCS控制系统");    
    showStatusMessage("系统已就绪");
}

MainWindow::~MainWindow()
{
    delete manualControl;
    delete nrtProcess;
    delete ui;
}

void MainWindow::setupConnections()
{
    connect(ui->pushButton, &QPushButton::pressed, this, &MainWindow::onPlusXPressed);
    connect(ui->pushButton, &QPushButton::released, this, &MainWindow::onPlusXReleased);
    connect(ui->pushButton_4, &QPushButton::pressed, this, &MainWindow::onMinusXPressed);
    connect(ui->pushButton_4, &QPushButton::released, this, &MainWindow::onMinusXReleased);
    connect(ui->pushButton_3, &QPushButton::pressed, this, &MainWindow::onPlusYPressed);
    connect(ui->pushButton_3, &QPushButton::released, this, &MainWindow::onPlusYReleased);
    connect(ui->pushButton_2, &QPushButton::pressed, this, &MainWindow::onMinusYPressed);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::onMinusYReleased);
    connect(ui->pushButton_5, &QPushButton::pressed, this, &MainWindow::onPlusZPressed);
    connect(ui->pushButton_5, &QPushButton::released, this, &MainWindow::onPlusZReleased);
    connect(ui->pushButton_11, &QPushButton::pressed, this, &MainWindow::onMinusZPressed);
    connect(ui->pushButton_11, &QPushButton::released, this, &MainWindow::onMinusZReleased);
    connect(ui->pushButton_13, &QPushButton::pressed, this, &MainWindow::onPlusAlphaPressed);
    connect(ui->pushButton_13, &QPushButton::released, this, &MainWindow::onPlusAlphaReleased);
    connect(ui->pushButton_15, &QPushButton::pressed, this, &MainWindow::onMinusAlphaPressed);
    connect(ui->pushButton_15, &QPushButton::released, this, &MainWindow::onMinusAlphaReleased);
    connect(ui->pushButton_16, &QPushButton::pressed, this, &MainWindow::onPlusBetaPressed);
    connect(ui->pushButton_16, &QPushButton::released, this, &MainWindow::onPlusBetaReleased);
    connect(ui->pushButton_14, &QPushButton::pressed, this, &MainWindow::onMinusBetaPressed);
    connect(ui->pushButton_14, &QPushButton::released, this, &MainWindow::onMinusBetaReleased);
    connect(ui->pushButton_enable, &QPushButton::clicked, this, &MainWindow::onEnableClicked);
    connect(ui->pushButton_reset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(ui->pushButton_disable, &QPushButton::clicked, this, &MainWindow::onDisableClicked);
    connect(ui->pushButton_errorClear, &QPushButton::clicked, this, &MainWindow::onerrorClear);
    connect(ui->pushButton_17, &QPushButton::clicked, this, &MainWindow::onXAxisHomeClicked);
    connect(ui->pushButton_18, &QPushButton::clicked, this, &MainWindow::onYAxisHomeClicked);
    connect(ui->pushButton_19, &QPushButton::clicked, this, &MainWindow::onZAxisHomeClicked);
    connect(ui->pushButton_20, &QPushButton::clicked, this, &MainWindow::onAlphaAxisHomeClicked);
    connect(ui->pushButton_21, &QPushButton::clicked, this, &MainWindow::onBetaAxisHomeClicked);
    connect(ui->pushButton_22, &QPushButton::clicked, this, &MainWindow::onAllAxisHomeClicked);
}

void MainWindow::onerrorClear()
{
    Command cmd_;
    std::strcpy(cmd_.cmd, "Reset");
    cmd_.args[EnableAxisId]=6;
    nrtProcess->shared_block_->commandQueue.push(cmd_);
}

void MainWindow::onDisableClicked()
{
    Command cmd_;
    std::strcpy(cmd_.cmd, "Disable");
    cmd_.args[EnableAxisId]=6;
    nrtProcess->shared_block_->commandQueue.push(cmd_);
}

void MainWindow::onResetClicked()
{
    nrtProcess->shared_block_->cmd.store(TaskScheduling::RESET,std::memory_order_release);
}

void MainWindow::onEnableClicked()
{
    Command cmd1;
    std::strcpy(cmd1.cmd, "Enable");
    cmd1.args[EnableAxisId]=6;
    nrtProcess->shared_block_->commandQueue.push(cmd1);
}

void MainWindow::onPlusXClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=0;
    cmd.args[JogjTargetPosition]=ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onPlusXPressed()
{
    currentMoveFunction = [this]() { onPlusXClicked(); };
    onPlusXClicked();
    moveTimer->start();
}

void MainWindow::onPlusXReleased()
{
    moveTimer->stop();
}

void MainWindow::onMinusXClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=0;
    cmd.args[JogjTargetPosition]=-ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onMinusXPressed()
{
    currentMoveFunction = [this]() { onMinusXClicked(); };
    onMinusXClicked();
    moveTimer->start();
}

void MainWindow::onMinusXReleased()
{
    moveTimer->stop();
}

void MainWindow::onPlusYPressed()
{
    currentMoveFunction = [this]() { onPlusYClicked(); };
    onPlusYClicked();
    moveTimer->start();
}

void MainWindow::onPlusYReleased()
{
    moveTimer->stop();
}

void MainWindow::onMinusYPressed()
{
    currentMoveFunction = [this]() { onMinusYClicked(); };
    onMinusYClicked();
    moveTimer->start();
}

void MainWindow::onMinusYReleased()
{
    moveTimer->stop();
}

void MainWindow::onPlusZPressed()
{
    currentMoveFunction = [this]() { onPlusZClicked(); };
    onPlusZClicked();
    moveTimer->start();
}

void MainWindow::onPlusZReleased()
{
    moveTimer->stop();
}

void MainWindow::onMinusZPressed()
{
    currentMoveFunction = [this]() { onMinusZClicked(); };
    onMinusZClicked();
    moveTimer->start();
}

void MainWindow::onMinusZReleased()
{
    moveTimer->stop();
}

void MainWindow::onPlusAlphaPressed()
{
    currentMoveFunction = [this]() { onPlusAlphaClicked(); };
    onPlusAlphaClicked();
    moveTimer->start();
}

void MainWindow::onPlusAlphaReleased()
{
    moveTimer->stop();
}

void MainWindow::onMinusAlphaPressed()
{
    currentMoveFunction = [this]() { onMinusAlphaClicked(); };
    onMinusAlphaClicked();
    moveTimer->start();
}

void MainWindow::onMinusAlphaReleased()
{
    moveTimer->stop();
}

void MainWindow::onPlusBetaPressed()
{
    currentMoveFunction = [this]() { onPlusBetaClicked(); };
    onPlusBetaClicked();
    moveTimer->start();
}

void MainWindow::onPlusBetaReleased()
{
    moveTimer->stop();
}

void MainWindow::onMinusBetaPressed()
{
    currentMoveFunction = [this]() { onMinusBetaClicked(); };
    onMinusBetaClicked();
    moveTimer->start();
}

void MainWindow::onMinusBetaReleased()
{
    moveTimer->stop();
}

void MainWindow::onTimerTimeout()
{
    if (currentMoveFunction) {
        currentMoveFunction();
    }
}

void MainWindow::onPlusYClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=1;
    cmd.args[JogjTargetPosition]=ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onMinusYClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=1;
    cmd.args[JogjTargetPosition]=-ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onPlusZClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=2;
    cmd.args[JogjTargetPosition]=ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onMinusZClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=2;
    cmd.args[JogjTargetPosition]=-ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onPlusAlphaClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=3;
    cmd.args[JogjTargetPosition]=ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onMinusAlphaClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=3;
    cmd.args[JogjTargetPosition]=-ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onPlusBetaClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=4;
    cmd.args[JogjTargetPosition]=ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onMinusBetaClicked()
{
    Command cmd;
    std::strcpy(cmd.cmd, "JogJ");
    cmd.args[JogjAxisId]=4;
    cmd.args[JogjTargetPosition]=-ui->lineEditlen->text().toInt();
    nrtProcess->shared_block_->commandQueue.push(cmd);
}

void MainWindow::onXAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "X轴回零操作已启动");
}

void MainWindow::onYAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "Y轴回零操作已启动");
}

void MainWindow::onZAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "Z轴回零操作已启动");
}

void MainWindow::onAlphaAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "α轴回零操作已启动");
}

void MainWindow::onBetaAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "β轴回零操作已启动");
}

void MainWindow::onAllAxisHomeClicked()
{
    QMessageBox::information(this, "回零操作", "所有轴回零操作已启动");
}

void MainWindow::showStatusMessage(const QString &message)
{
    if (ui->statusbar) {
        ui->statusbar->showMessage(message, 3000);
    }
}

void MainWindow::addJogJ()
{
    nrtProcess->shared_block_->Multiplied.store(1,std::memory_order_release);
    currentMoveFunction = [this]() {
        singleAxisContinueMotion motion;
        motion.motion=true;
        motion.direction=true;
        nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
    };
    currentMoveFunction();
    moveTimer->start();
}

void MainWindow::deleteJogJ()
{
    singleAxisContinueMotion motion;
    motion.motion=false;
    nrtProcess->shared_block_->sacm.store(motion,std::memory_order_release);
    moveTimer->stop();
}

// ZMQ 槽函数
void MainWindow::onZMQConnected()
{
    useZMQ_ = true;
    showStatusMessage("已连接到 NRT 进程 (ZMQ)");
}

void MainWindow::onZMQDisconnected()
{
    useZMQ_ = false;
    showStatusMessage("已断开连接，使用共享内存模式");
}

void MainWindow::onZMQCommandSent(const QString& command, bool success)
{
    if (!success) {
        showStatusMessage(QString("命令失败: %1").arg(command));
    }
}

void MainWindow::onZMQError(const QString& error)
{
    showStatusMessage(QString("错误: %1").arg(error));
}

void MainWindow::sendMotionCommand(const QString& command, const QVector<double>& args)
{
    if (useZMQ_ && zmqClient->isConnected()) {
        zmqClient->sendCommand(command, args);
    } else {
        Command cmd;
        strncpy(cmd.cmd, command.toStdString().c_str(), sizeof(cmd.cmd) - 1);
        cmd.cmd[sizeof(cmd.cmd) - 1] = '\0';
        for (size_t i = 0; i < args.size() && i < 10; ++i) {
            cmd.args[i] = args[i];
        }
        nrtProcess->shared_block_->commandQueue.push(cmd);
    }
}
