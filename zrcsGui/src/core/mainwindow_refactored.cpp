#include "core/mainwindow_refactored.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QGroupBox>
#include <QFont>
#include <functional>

MainWindowRefactored::MainWindowRefactored(QWidget *parent)
    : QMainWindow(parent), useZMQ(true), currentOverride(100.0), currentStepSize(0.1)
{
    setWindowTitle("ZRCS 机器人控制系统 v2.0");
    
    setupUI();
    setupConnections();
    setupStyles();
    
    nrtProcess = new NRTProcess();
    zmqClient = new ZMQClient();
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindowRefactored::onUpdateTimer);
    updateTimer->start(100);
    
    resize(1400, 900);
    showMaximized();
}

MainWindowRefactored::~MainWindowRefactored()
{
    if (updateTimer) updateTimer->stop();
}

void MainWindowRefactored::setupUI()
{
    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // 左侧面板
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    // 状态指示器
    globalStatus = new StatusIndicator();
    leftLayout->addWidget(globalStatus);
    
    // 轴位置显示
    QGroupBox *posGroup = new QGroupBox("轴位置");
    posGroup->setStyleSheet("color: #00FF00; border: 1px solid #444;");
    QVBoxLayout *posLayout = new QVBoxLayout();
    
    for (int i = 0; i < 5; ++i) {
        AxisPositionDisplay *display = new AxisPositionDisplay(QString("轴 %1").arg(i + 1));
        axisDisplays.append(display);
        posLayout->addWidget(display);
    }
    
    posGroup->setLayout(posLayout);
    leftLayout->addWidget(posGroup);
    
    // 手动控制
    createJogControl();
    if (jogPanel) {
        leftLayout->addWidget(jogPanel);
    }
    
    // IO 面板
    createIOPanel();
    if (ioPanel) {
        leftLayout->addWidget(ioPanel);
    }
    
    leftLayout->addStretch();
    
    QWidget *leftWidget = new QWidget();
    leftWidget->setLayout(leftLayout);
    mainLayout->addWidget(leftWidget, 1);
    
    // 右侧选项卡
    advancedTabs = new QTabWidget();
    createTrajectoryPanel();
    createAlarmPanel();
    createAdvancedModules();
    
    mainLayout->addWidget(advancedTabs, 2);
    
    // 状态栏
    zmqStatusLabel = new QLabel("ZMQ: 未连接");
    etherCATStatusLabel = new QLabel("EtherCAT: 未连接");
    homedLabel = new QLabel("归零: 否");
    servoLabel = new QLabel("伺服: 关");
    
    statusBar()->addWidget(zmqStatusLabel);
    statusBar()->addWidget(etherCATStatusLabel);
    statusBar()->addWidget(homedLabel);
    statusBar()->addWidget(servoLabel);
}

void MainWindowRefactored::setupConnections()
{
    if (jogPanel) {
        connect(jogPanel, &JogControlPanel::jogPressed, this, &MainWindowRefactored::onJogPressed);
        connect(jogPanel, &JogControlPanel::jogReleased, this, &MainWindowRefactored::onJogReleased);
    }
    if (ioPanel) {
        connect(ioPanel, &IOPanel::outputToggled, this, &MainWindowRefactored::onOutputToggled);
    }
}

void MainWindowRefactored::setupStyles()
{
    setStyleSheet(
        "QMainWindow { background-color: #1a1a1a; }"
        "QLabel { color: #00FF00; }"
        "QPushButton { background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 5px; }"
        "QGroupBox { color: #00FF00; border: 1px solid #444; padding: 5px; }"
        "QTabWidget { background-color: #1a1a1a; }"
        "QTabBar::tab { background-color: #2a2a2a; color: #00FF00; padding: 5px; }"
        "QTabBar::tab:selected { background-color: #3a3a3a; }"
    );
}

void MainWindowRefactored::createPositionDisplay()
{
    // 已在 setupUI 中实现
}

void MainWindowRefactored::createJogControl()
{
    jogPanel = new JogControlPanel();
}

void MainWindowRefactored::createTrajectoryPanel()
{
    trajectoryPanel = new TrajectoryPanel();
    advancedTabs->addTab(trajectoryPanel, "轨迹可视化");
}

void MainWindowRefactored::createAlarmPanel()
{
    alarmPanel = new AlarmPanel();
    advancedTabs->addTab(alarmPanel, "告警日志");
}

void MainWindowRefactored::createIOPanel()
{
    ioPanel = new IOPanel();
}

void MainWindowRefactored::createSettingsPanel() {}

void MainWindowRefactored::createAdvancedModules()
{
    gcodePanel = new GCodePanel();
    advancedTabs->addTab(gcodePanel, "G-code 编辑器");
    
    remotePanel = new RemoteMonitorPanel();
    advancedTabs->addTab(remotePanel, "远程监控");
    
    pluginPanel = new PluginPanel();
    advancedTabs->addTab(pluginPanel, "插件管理");
}

void MainWindowRefactored::onUpdateTimer() { updateGlobalStatus(); }
void MainWindowRefactored::updateGlobalStatus() {}
void MainWindowRefactored::updateCommunicationStatus() {}
void MainWindowRefactored::onJogPressed(int /*axis*/, int /*direction*/) {}
void MainWindowRefactored::onJogReleased(int /*axis*/) {}
void MainWindowRefactored::onStepSizeChanged(double size) { currentStepSize = size; }
void MainWindowRefactored::onOverrideChanged(int percent) { currentOverride = percent; }
void MainWindowRefactored::onHomeRequested(int /*axis*/) {}
void MainWindowRefactored::onHomeAllRequested() {}
void MainWindowRefactored::onOutputToggled(int /*index*/, bool /*state*/) {}
void MainWindowRefactored::onZMQConnected() { zmqStatusLabel->setText("ZMQ: 已连接"); }
void MainWindowRefactored::onZMQDisconnected() { zmqStatusLabel->setText("ZMQ: 未连接"); }
void MainWindowRefactored::onZMQError(const QString &/*error*/) {}
void MainWindowRefactored::sendMotionCommand(const QString &/*command*/, const QVector<double> &/*args*/) {}
void MainWindowRefactored::showConfirmDialog(const QString &/*title*/, const QString &/*message*/, std::function<void()> /*onConfirm*/) {}
