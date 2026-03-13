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
}

void MainWindowRefactored::onUpdateTimer() { updateGlobalStatus(); }
void MainWindowRefactored::updateGlobalStatus() {}
void MainWindowRefactored::updateCommunicationStatus() {}
void MainWindowRefactored::onJogPressed(int, int) {}
void MainWindowRefactored::onJogReleased(int) {}
void MainWindowRefactored::onStepSizeChanged(double size) { currentStepSize = size; }
void MainWindowRefactored::onOverrideChanged(int percent) { currentOverride = percent; }
void MainWindowRefactored::onHomeRequested(int) {}
void MainWindowRefactored::onHomeAllRequested() {}
void MainWindowRefactored::onSetCurrentAsOriginRequested(int) {}
void MainWindowRefactored::onOutputToggled(int, bool) {}
void MainWindowRefactored::onZMQConnected() { zmqStatusLabel->setText("ZMQ: 已连接"); }
void MainWindowRefactored::onZMQDisconnected() { zmqStatusLabel->setText("ZMQ: 未连接"); }
void MainWindowRefactored::onZMQError(const QString &) {}
void MainWindowRefactored::sendMotionCommand(const QString &, const QVector<double> &) {}
void MainWindowRefactored::showConfirmDialog(const QString &, const QString &, std::function<void()>) {}
