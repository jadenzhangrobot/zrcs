#include "core/MainWindow.h"
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
#include "ui_main_window.h"

MainWindowRefactored::MainWindowRefactored(QWidget *parent)
    : QMainWindow(parent), useZMQ(true), currentOverride(100.0), currentStepSize(0.0)
{
    setWindowTitle("ZRCS 机器人控制系统 v2.0");
    
    setupUI();
    setupConnections();
    setupStyles();
    
    nrtProcess = new NRTProcess();
    zmqClient = new ZMQClient();

    // ZMQ 状态信号连接
    connect(zmqClient, &ZMQClient::connected, this, &MainWindowRefactored::onZMQConnected);
    connect(zmqClient, &ZMQClient::disconnected, this, &MainWindowRefactored::onZMQDisconnected);
    connect(zmqClient, &ZMQClient::errorOccurred, this, &MainWindowRefactored::onZMQError);
    zmqClient->connectToServer();

    // 状态订阅器 (ZMQ SUB port 5556)
    const auto& commCfg2 = ZrcsConfig::Config::instance().comm;
    statusSubscriber = new ZMQStatusSubscriber(commCfg2.zmqHost, 5556, this);
    connect(statusSubscriber, &ZMQStatusSubscriber::axisPositionsUpdated,
            this, &MainWindowRefactored::onAxisPositionsUpdated);
    statusSubscriber->start();

    // 连接命令面板信号
    if (commandPanel) {
        connect(commandPanel, &CommandPanel::commandRequested,
                this, &MainWindowRefactored::sendMotionCommand);
    }

    // 连接行为树面板信号到 ZMQ 客户端
    if (behaviorTreePanel && zmqClient) {
        QObject::connect(behaviorTreePanel, &BehaviorTreePanel::requestBTLoad,
            zmqClient, [this](const QString& xml) {
                zmqClient->sendBTCommand("LOAD", xml);
            });
        QObject::connect(behaviorTreePanel, &BehaviorTreePanel::requestBTStart,
            zmqClient, [this]() {
                zmqClient->sendBTCommand("START");
            });
        QObject::connect(behaviorTreePanel, &BehaviorTreePanel::requestBTStop,
            zmqClient, [this]() {
                zmqClient->sendBTCommand("STOP");
            });
    }
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindowRefactored::onUpdateTimer);
    updateTimer->start(100);
    
    showMaximized();
}

MainWindowRefactored::~MainWindowRefactored()
{
    if (updateTimer) updateTimer->stop();
    if (statusSubscriber) statusSubscriber->stop();
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

    // 连接控件
    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    ipInput = new QLineEdit(commCfg.zmqHost);
    ipInput->setPlaceholderText("192.168.x.x");
    ipInput->setFixedWidth(140);

    connectBtn = new QPushButton("连接");
    connectBtn->setFixedWidth(60);
    connect(connectBtn, &QPushButton::clicked, this, &MainWindowRefactored::onConnectClicked);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(new QLabel("IP:"));
    statusBar()->addWidget(ipInput);
    statusBar()->addWidget(connectBtn);
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
        connect(jogPanel, &JogControlPanel::stepSizeChanged, this, &MainWindowRefactored::onStepSizeChanged);
        connect(jogPanel, &JogControlPanel::overrideChanged, this, &MainWindowRefactored::onOverrideChanged);
        connect(jogPanel, &JogControlPanel::homeRequested, this, &MainWindowRefactored::onHomeRequested);
        connect(jogPanel, &JogControlPanel::homeAllRequested, this, &MainWindowRefactored::onHomeAllRequested);
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
    commandPanel = findChild<CommandPanel*>("commandPanel");
}

void MainWindowRefactored::createQuickActions()
{
    quickActionGroup = findChild<QGroupBox*>("quickActionGroup");
    quickActionLayout = findChild<QGridLayout*>("quickActionLayout");
    if (!quickActionGroup || !quickActionLayout) return;

    // 添加默认按钮
    auto *btnServo = addQuickAction("伺服使能");
    btnServo->setCheckable(true);
    connect(btnServo, &QPushButton::toggled, this, [this](bool on) {
        servoLabel->setText(on ? "伺服: 开" : "伺服: 关");
        int axisCount = ZrcsConfig::Config::instance().ui.axisCount;
        if (on) {
            sendMotionCommand("SYS_RUN");
            sendMotionCommand("Enable", {static_cast<double>(axisCount)});
        } else {
            sendMotionCommand("Disable", {static_cast<double>(axisCount)});
        }
    });

    auto *btnRun = addQuickAction("运行程序");
    connect(btnRun, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_RUN");
    });

    auto *btnPause = addQuickAction("暂停");
    connect(btnPause, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_STOP");
    });

    auto *btnStop = addQuickAction("停止");
    btnStop->setProperty("kind", "danger");
    connect(btnStop, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_JOG_STOP");
        sendMotionCommand("SYS_STOP");
    });

    auto *btnEStop = addQuickAction("急停");
    btnEStop->setProperty("kind", "danger");
    connect(btnEStop, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_ESTOP");
        // 同步伺服按钮状态
        for (auto *btn : quickActionGroup->findChildren<QPushButton*>()) {
            if (btn->isCheckable()) { btn->setChecked(false); break; }
        }
    });

    auto *btnReset = addQuickAction("复位");
    connect(btnReset, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_RESET");
    });
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
void MainWindowRefactored::onJogPressed(int axis, int direction)
{
    if (currentStepSize == 0.0) {
        double dirFlag = (direction > 0) ? 1.0 : 0.0;
        sendMotionCommand("SYS_JOG_START", {static_cast<double>(axis), dirFlag});
    } else {
        double distance = currentStepSize * direction;
        sendMotionCommand("JogJ", {static_cast<double>(axis), distance});
    }
}

void MainWindowRefactored::onJogReleased(int axis)
{
    Q_UNUSED(axis);
    if (currentStepSize == 0.0) {
        sendMotionCommand("SYS_JOG_STOP");
    }
}
void MainWindowRefactored::onStepSizeChanged(double size) { currentStepSize = size; }
void MainWindowRefactored::onOverrideChanged(int percent)
{
    currentOverride = percent;
    sendMotionCommand("SYS_SET_MULTIPLIER", {static_cast<double>(percent)});
}

void MainWindowRefactored::onHomeRequested(int axis)
{
    sendMotionCommand("JogabsJ", {static_cast<double>(axis), 0.0});
}

void MainWindowRefactored::onHomeAllRequested()
{
    sendMotionCommand("Movehome");
}

void MainWindowRefactored::onSetCurrentAsOriginRequested(int axis)
{
    sendMotionCommand("SetZero", {static_cast<double>(axis)});
}

void MainWindowRefactored::onOutputToggled(int index, bool state)
{
    sendMotionCommand("SetDO", {0.0, static_cast<double>(index), state ? 1.0 : 0.0});
}
void MainWindowRefactored::onZMQConnected()
{
    zmqStatusLabel->setText("ZMQ: 已连接");
    connectBtn->setText("断开");
    ipInput->setEnabled(false);
}
void MainWindowRefactored::onZMQDisconnected()
{
    zmqStatusLabel->setText("ZMQ: 未连接");
    connectBtn->setText("连接");
    ipInput->setEnabled(true);
}
void MainWindowRefactored::onZMQError(const QString &error)
{
    qDebug() << "[GUI] ZMQ error:" << error;
    zmqStatusLabel->setText("ZMQ: 错误");
}

void MainWindowRefactored::sendMotionCommand(const QString &command, const QVector<double> &args)
{
    if (!zmqClient || !zmqClient->isConnected()) {
        qDebug() << "[GUI] ZMQ not connected, cannot send:" << command;
        return;
    }
    zmqClient->sendCommand(command, args);
}

void MainWindowRefactored::showConfirmDialog(const QString &title, const QString &message, std::function<void()> onConfirm)
{
    auto result = QMessageBox::question(this, title, message,
                                         QMessageBox::Yes | QMessageBox::No,
                                         QMessageBox::No);
    if (result == QMessageBox::Yes && onConfirm) {
        onConfirm();
    }
}

void MainWindowRefactored::onAxisPositionsUpdated(QVector<double> positions)
{
    if (!jogPanel) return;
    for (int i = 0; i < positions.size(); ++i) {
        jogPanel->setAxisPosition(i, positions[i]);
    }
}

void MainWindowRefactored::onConnectClicked()
{
    if (zmqClient && zmqClient->isConnected()) {
        zmqClient->disconnectFromServer();
        if (statusSubscriber) statusSubscriber->stop();
        return;
    }

    QString host = ipInput->text().trimmed();
    if (host.isEmpty()) {
        host = "localhost";
        ipInput->setText(host);
    }

    // 保存到配置
    auto& commCfg = ZrcsConfig::Config::instance().comm;
    commCfg.zmqHost = host;

    // 重建 ZMQClient (固定端口 5555)
    if (zmqClient) {
        zmqClient->disconnectFromServer();
        delete zmqClient;
    }

    zmqClient = new ZMQClient(host, 5555);
    connect(zmqClient, &ZMQClient::connected, this, &MainWindowRefactored::onZMQConnected);
    connect(zmqClient, &ZMQClient::disconnected, this, &MainWindowRefactored::onZMQDisconnected);
    connect(zmqClient, &ZMQClient::errorOccurred, this, &MainWindowRefactored::onZMQError);

    // 重连命令面板
    if (commandPanel) {
        disconnect(commandPanel, &CommandPanel::commandRequested,
                   this, &MainWindowRefactored::sendMotionCommand);
        connect(commandPanel, &CommandPanel::commandRequested,
                this, &MainWindowRefactored::sendMotionCommand);
    }

    zmqClient->connectToServer();
    zmqStatusLabel->setText(QString("ZMQ: 连接中 %1:5555").arg(host));

    // 重建 StatusSubscriber (固定端口 5556)
    if (statusSubscriber) {
        statusSubscriber->stop();
        delete statusSubscriber;
    }
    statusSubscriber = new ZMQStatusSubscriber(host, 5556, this);
    connect(statusSubscriber, &ZMQStatusSubscriber::axisPositionsUpdated,
            this, &MainWindowRefactored::onAxisPositionsUpdated);
    statusSubscriber->start();
}
