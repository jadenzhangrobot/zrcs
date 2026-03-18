#include "core/mainwindow_refactored.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QGroupBox>
#include <QFont>
#include <QScrollArea>
#include <QFrame>
#include <functional>
#include <QFile>
#include "ui_mainwindow_refactored.h"

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
    
    showMaximized();
}

MainWindowRefactored::~MainWindowRefactored()
{
    if (updateTimer) updateTimer->stop();
}

void MainWindowRefactored::setupUI()
{
    Ui::MainWindowRefactored ui;
    ui.setupUi(this);

    QHBoxLayout *mainLayout = findChild<QHBoxLayout*>("mainLayout");
    advancedTabs = findChild<QTabWidget*>("mainTabs");
    if (mainLayout) {
        mainLayout->setStretch(0, 0);
        mainLayout->setStretch(1, 1);
    }
    globalStatus = findChild<StatusIndicator*>("globalStatusIndicator");
    jogPanel = findChild<JogControlPanel*>("jogPanel");
    createJogControl();
    createQuickActions();
    createTrajectoryPanel();
    createAlarmPanel();
    createAdvancedModules();

    zmqStatusLabel = new QLabel("ZMQ: 未连接");
    etherCATStatusLabel = new QLabel("EtherCAT: 未连接");
    homedLabel = new QLabel("归零: 否");
    servoLabel = new QLabel("伺服: 关");
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(zmqStatusLabel);
    statusBar()->addPermanentWidget(etherCATStatusLabel);
    statusBar()->addPermanentWidget(homedLabel);
    statusBar()->addPermanentWidget(servoLabel);
}

void MainWindowRefactored::setupConnections()
{
    if (jogPanel) {
        connect(jogPanel, &JogControlPanel::jogPressed, this, &MainWindowRefactored::onJogPressed);
        connect(jogPanel, &JogControlPanel::jogReleased, this, &MainWindowRefactored::onJogReleased);
        connect(jogPanel, &JogControlPanel::setCurrentAsOriginRequested, this, &MainWindowRefactored::onSetCurrentAsOriginRequested);
    }
    if (ioMonitorPanel) {
        connect(ioMonitorPanel, &IOPanel::outputToggled, this, &MainWindowRefactored::onOutputToggled);
    }
}

void MainWindowRefactored::setupStyles()
{
    QFile qssFile(QStringLiteral(ZRCSGUI_SOURCE_DIR "/resources/style/dark_theme.qss"));
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    } else {
        setStyleSheet("");
    }
}

void MainWindowRefactored::createPositionDisplay()
{
    // 已在 setupUI 中实现
}

void MainWindowRefactored::createJogControl()
{
    jogPanel = findChild<JogControlPanel*>("jogPanel");
}

void MainWindowRefactored::createTrajectoryPanel()
{
    trajectoryPanel = findChild<TrajectoryPanel*>("trajectoryPanel");
}

void MainWindowRefactored::createAlarmPanel()
{
    alarmPanel = findChild<AlarmPanel*>("alarmPanel");
}

void MainWindowRefactored::createIOPanel()
{
    ioPanel = findChild<IOPanel*>("ioPanel");
}

void MainWindowRefactored::createSettingsPanel() {}

void MainWindowRefactored::createAdvancedModules()
{
    gcodePanel = findChild<GCodePanel*>("gcodePanel");
    ioMonitorPanel = findChild<IOPanel*>("ioMonitorPanel");
    remotePanel = findChild<RemoteMonitorPanel*>("remotePanel");
    pluginPanel = findChild<PluginPanel*>("pluginPanel");
    behaviorTreePanel = findChild<BehaviorTreePanel*>("behaviorTreePanel");
}

void MainWindowRefactored::createQuickActions()
{
    quickActionGroup = findChild<QGroupBox*>("quickActionGroup");
    quickActionLayout = findChild<QGridLayout*>("quickActionLayout");
    if (!quickActionGroup || !quickActionLayout) return;

    // 添加默认按钮 - 可根据需要修改
    auto *btnServo = addQuickAction("伺服使能");
    btnServo->setCheckable(true);
    connect(btnServo, &QPushButton::toggled, this, [this](bool on) {
        servoLabel->setText(on ? "伺服: 开" : "伺服: 关");
    });

    auto *btnRun = addQuickAction("运行程序");
    connect(btnRun, &QPushButton::clicked, this, [](){ /* TODO */ });

    auto *btnPause = addQuickAction("暂停");
    connect(btnPause, &QPushButton::clicked, this, [](){ /* TODO */ });

    auto *btnStop = addQuickAction("停止");
    btnStop->setProperty("kind", "danger");
    connect(btnStop, &QPushButton::clicked, this, [](){ /* TODO */ });

    auto *btnEStop = addQuickAction("急停");
    btnEStop->setProperty("kind", "danger");
    connect(btnEStop, &QPushButton::clicked, this, [](){ /* TODO */ });

    auto *btnReset = addQuickAction("复位");
    connect(btnReset, &QPushButton::clicked, this, [](){ /* TODO */ });
}

QPushButton* MainWindowRefactored::addQuickAction(const QString &text, const QString &iconPath)
{
    if (!quickActionLayout) return nullptr;

    auto *btn = new QPushButton(text, quickActionGroup);
    btn->setMinimumHeight(36);
    if (!iconPath.isEmpty()) {
        btn->setIcon(QIcon(iconPath));
    }

    int count = quickActionLayout->count();
    int cols = 3;
    int row = count / cols;
    int col = count % cols;
    quickActionLayout->addWidget(btn, row, col);
    return btn;
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
void MainWindowRefactored::onSetCurrentAsOriginRequested(int /*axis*/) {}
void MainWindowRefactored::onOutputToggled(int /*index*/, bool /*state*/) {}
void MainWindowRefactored::onZMQConnected() { zmqStatusLabel->setText("ZMQ: 已连接"); }
void MainWindowRefactored::onZMQDisconnected() { zmqStatusLabel->setText("ZMQ: 未连接"); }
void MainWindowRefactored::onZMQError(const QString &/*error*/) {}
void MainWindowRefactored::sendMotionCommand(const QString &/*command*/, const QVector<double> &/*args*/) {}
void MainWindowRefactored::showConfirmDialog(const QString &/*title*/, const QString &/*message*/, std::function<void()> /*onConfirm*/) {}
