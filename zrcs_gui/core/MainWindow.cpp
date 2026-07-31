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
#include <QDateTime>
#include <functional>
#include <QFile>
#include "ui_main_window.h"
#include "resources/style/StyleLoader.h"

MainWindowRefactored::MainWindowRefactored(QWidget *parent)
    : QMainWindow(parent),
      useZMQ(true),
      controlPanelExpanded_(true),
      currentOverride(100.0),
      currentStepSize(0.0)
{
    setWindowTitle("ZRCS 机器人控制系统 v2.0");
    setProperty("currentAxisCount", 0);
    
    setupUI();
    setupConnections();
    setupStyles();
    
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
    connect(statusSubscriber, &ZMQStatusSubscriber::taskSchedulingUpdated,
            this, &MainWindowRefactored::onTaskSchedulingUpdated);
    connect(statusSubscriber, &ZMQStatusSubscriber::rtLogReceived,
            this, &MainWindowRefactored::onRtLogReceived);
    connect(statusSubscriber, &ZMQStatusSubscriber::btStatusUpdated,
            this, &MainWindowRefactored::onBtStatusUpdated);
    statusSubscriber->start();

    // 连接命令面板信号
    if (commandPanel) {
        connect(commandPanel, &CommandPanel::commandRequested,
                this, &MainWindowRefactored::onCommandPanelCommandRequested);
    }

    bindBehaviorTreeClient();
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindowRefactored::onUpdateTimer);
    updateTimer->start(100);
    
    showMaximized();
}

MainWindowRefactored::~MainWindowRefactored()
{
    if (updateTimer) updateTimer->stop();
    if (statusSubscriber) statusSubscriber->stop();
    delete statusSubscriber;
    statusSubscriber = nullptr;
    delete zmqClient;
    zmqClient = nullptr;
}

void MainWindowRefactored::setupUI()
{
    Ui::MainWindowRefactored ui;
    ui.setupUi(this);

    QHBoxLayout *mainLayout = findChild<QHBoxLayout*>("mainLayout");
    advancedTabs = findChild<QTabWidget*>("mainTabs");
    controlSidebarHost = findChild<QWidget*>("leftSidebarHost");
    controlScrollArea = findChild<QScrollArea*>("leftScrollArea");
    controlPanelToggleButton = findChild<QPushButton*>("btnControlPanelToggle");
    if (mainLayout) {
        mainLayout->setStretch(0, 0);
        mainLayout->setStretch(1, 1);
    }
    globalStatus = findChild<StatusIndicator*>("globalStatusIndicator");
    jogPanel = findChild<JogControlPanel*>("jogPanel");
    auto *statusBarIpLabel = findChild<QLabel*>("statusBarIpLabel");
    ipInput = findChild<QLineEdit*>("statusBarIpInput");
    connectBtn = findChild<QPushButton*>("statusBarConnectBtn");
    schedStateLabel = findChild<QLabel*>("schedStateLabel");
    zmqStatusLabel = findChild<QLabel*>("zmqStatusLabel");
    etherCATStatusLabel = findChild<QLabel*>("etherCATStatusLabel");
    homedLabel = findChild<QLabel*>("homedLabel");
    servoLabel = findChild<QLabel*>("servoLabel");

    createJogControl();
    createQuickActions();
    createMujocoPanel();
    createAlarmPanel();
    createAdvancedModules();

    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    if (ipInput) {
        ipInput->setText(commCfg.zmqHost);
    }

    if (connectBtn) {
        connect(connectBtn, &QPushButton::clicked, this, &MainWindowRefactored::onConnectClicked);
    }

    if (controlPanelToggleButton) {
        connect(controlPanelToggleButton, &QPushButton::clicked, this, [this]() {
            setControlPanelExpanded(!controlPanelExpanded_);
        });
    }

    setControlPanelExpanded(true);

    if (statusBar() && statusBarIpLabel && ipInput && connectBtn && schedStateLabel &&
        zmqStatusLabel && etherCATStatusLabel && homedLabel && servoLabel) {
        statusBar()->addWidget(statusBarIpLabel);
        statusBar()->addWidget(ipInput);
        statusBar()->addWidget(connectBtn);
        statusBar()->addPermanentWidget(schedStateLabel);
        statusBar()->addPermanentWidget(zmqStatusLabel);
        statusBar()->addPermanentWidget(etherCATStatusLabel);
        statusBar()->addPermanentWidget(homedLabel);
        statusBar()->addPermanentWidget(servoLabel);
    }
}

void MainWindowRefactored::setControlPanelExpanded(bool expanded)
{
    controlPanelExpanded_ = expanded;

    auto *mainLayout = findChild<QHBoxLayout*>("mainLayout");
    auto *sidebarLayout = findChild<QHBoxLayout*>("leftSidebarLayout");
    const int collapsedWidth = 28;

    if (controlScrollArea) {
        controlScrollArea->setVisible(expanded);
        controlScrollArea->setMinimumWidth(expanded ? 480 : 0);
        controlScrollArea->setMaximumWidth(expanded ? 620 : 0);
    }

    if (controlSidebarHost) {
        controlSidebarHost->setMinimumWidth(expanded ? 0 : collapsedWidth);
        controlSidebarHost->setMaximumWidth(expanded ? QWIDGETSIZE_MAX : collapsedWidth);
    }

    if (sidebarLayout) {
        sidebarLayout->setContentsMargins(expanded ? 0 : 6, 0, expanded ? 0 : 6, 0);
        sidebarLayout->setSpacing(expanded ? 0 : 0);
    }

    if (mainLayout) {
        mainLayout->setSpacing(expanded ? 0 : 8);
    }

    if (controlPanelToggleButton) {
        controlPanelToggleButton->setText(QString());
        controlPanelToggleButton->setToolTip(expanded ? "隐藏控制界面" : "显示控制界面");
    }
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
}

void MainWindowRefactored::setupStyles()
{
    QFile qssFile(QStringLiteral(":/style/dark_theme.qss"));
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    } else {
        qWarning() << "Failed to load embedded dark theme:" << qssFile.errorString();
        setStyleSheet(StyleLoader::getDarkThemeStyleSheet());
    }
}

void MainWindowRefactored::createJogControl()
{
    jogPanel = findChild<JogControlPanel*>("jogPanel");
}

void MainWindowRefactored::createMujocoPanel()
{
    mujocoPanel = findChild<MujocoPanel*>("mujocoPanel");
    if (advancedTabs && mujocoPanel) {
        const int index = advancedTabs->indexOf(mujocoPanel->parentWidget());
        if (index >= 0) {
            advancedTabs->setTabText(index, QStringLiteral("仿真界面"));
        }
    }
}

void MainWindowRefactored::createAlarmPanel()
{
    alarmPanel = findChild<AlarmPanel*>("alarmPanel");
}

void MainWindowRefactored::createSettingsPanel() {}

void MainWindowRefactored::createAdvancedModules()
{
    behaviorTreePanel = findChild<BehaviorTreePanel*>("behaviorTreePanel");
}

void MainWindowRefactored::createQuickActions()
{
    quickActionGroup = findChild<QGroupBox*>("quickActionGroup");
    commandPanel = findChild<CommandPanel*>("commandPanel");
    auto *btnRun = findChild<QPushButton*>("btnQuickRun");
    auto *btnPause = findChild<QPushButton*>("btnQuickPause");
    auto *btnEStop = findChild<QPushButton*>("btnQuickEStop");
    auto *btnReset = findChild<QPushButton*>("btnQuickReset");
    if (!quickActionGroup || !btnRun || !btnPause || !btnEStop || !btnReset) {
        return;
    }

    connect(btnRun, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_RUN");
    });

    connect(btnPause, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_STOP");
    });

    connect(btnEStop, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_ESTOP");
    });

    connect(btnReset, &QPushButton::clicked, this, [this]() {
        sendMotionCommand("SYS_RESET");
    });
}

void MainWindowRefactored::bindBehaviorTreeClient()
{
    if (!behaviorTreePanel || !zmqClient) {
        return;
    }

    disconnect(behaviorTreePanel, &BehaviorTreePanel::requestBTLoad, nullptr, nullptr);
    disconnect(behaviorTreePanel, &BehaviorTreePanel::requestBTStart, nullptr, nullptr);
    disconnect(behaviorTreePanel, &BehaviorTreePanel::requestBTStop, nullptr, nullptr);

    connect(behaviorTreePanel, &BehaviorTreePanel::requestBTLoad,
            this, [this](const QString& xml) {
                if (zmqClient) {
                    zmqClient->sendBTCommand("LOAD", xml);
                }
            });
    connect(behaviorTreePanel, &BehaviorTreePanel::requestBTStart,
            this, [this]() {
                if (zmqClient) {
                    zmqClient->sendBTCommand("START");
                }
            });
    connect(behaviorTreePanel, &BehaviorTreePanel::requestBTStop,
            this, [this]() {
                if (zmqClient) {
                    zmqClient->sendBTCommand("STOP");
                }
            });
}

void MainWindowRefactored::onUpdateTimer()
{
    updateGlobalStatus();
}
void MainWindowRefactored::updateGlobalStatus() {}
void MainWindowRefactored::updateCommunicationStatus() {}

void MainWindowRefactored::onBtStatusUpdated(const QString &treeState,
                                             const QString &currentNode,
                                             const QString &message)
{
    if (behaviorTreePanel) {
        behaviorTreePanel->onBtStatusUpdated(treeState, currentNode, message);
    }
}
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
    if (mujocoPanel) {
        mujocoPanel->updateAxisPositions(positions);
    }

    if (!jogPanel) return;

    const int axisCount = positions.size();
    if (axisCount > 0 && property("currentAxisCount").toInt() != axisCount) {
        setProperty("currentAxisCount", axisCount);
        jogPanel->setAxisCount(axisCount);
    }

    for (int i = 0; i < positions.size(); ++i) {
        jogPanel->setAxisPosition(i, positions[i]);
    }
}

void MainWindowRefactored::onCommandPanelCommandRequested(const QString &cmd, const QVector<double> &args)
{
    QStringList argTexts;
    argTexts.reserve(args.size());
    for (double arg : args) {
        argTexts.append(QString::number(arg, 'f', 3));
    }

    if (alarmPanel) {
        alarmPanel->appendOperationLog(
            QString("[%1] %2(%3)")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                .arg(cmd)
                .arg(argTexts.join(", ")));
    }

    sendMotionCommand(cmd, args);
}

void MainWindowRefactored::onTaskSchedulingUpdated(const QString &state)
{
    schedStateLabel->setText(QString("状态: %1").arg(state));

    // 同步 StatusIndicator
    if (state == "RUN") {
        globalStatus->setState(StatusIndicator::Running);
    } else if (state == "ERROR") {
        globalStatus->setState(StatusIndicator::Alarm);
    } else {
        globalStatus->setState(StatusIndicator::Idle);
    }
}

void MainWindowRefactored::onRtLogReceived(quint32 level, const QString &message, const QString &timestamp)
{
    if (!alarmPanel) {
        return;
    }

    const QString text = QString("[%1] %2").arg(timestamp, message.trimmed());
    if (level == 0) {
        alarmPanel->appendOperationLog(text);
    } else {
        alarmPanel->addAlarm(message.trimmed(), timestamp,
                             level == 1 ? QStringLiteral("警告") : QStringLiteral("错误"));
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
                   this, &MainWindowRefactored::onCommandPanelCommandRequested);
        connect(commandPanel, &CommandPanel::commandRequested,
                this, &MainWindowRefactored::onCommandPanelCommandRequested);
    }
    bindBehaviorTreeClient();

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
    connect(statusSubscriber, &ZMQStatusSubscriber::taskSchedulingUpdated,
            this, &MainWindowRefactored::onTaskSchedulingUpdated);
    connect(statusSubscriber, &ZMQStatusSubscriber::rtLogReceived,
            this, &MainWindowRefactored::onRtLogReceived);
    connect(statusSubscriber, &ZMQStatusSubscriber::btStatusUpdated,
            this, &MainWindowRefactored::onBtStatusUpdated);
    statusSubscriber->start();
}
