#include "remote/remoteMonitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QStyle>
#include "ui_remote_data_monitor.h"
#include "ui_remote_monitor_panel.h"

// ============================================================================
// ZMQDataReceiver 实现
// ============================================================================

ZMQDataReceiver::ZMQDataReceiver(const QString &endpoint)
    : endpoint(endpoint), running(false), context(nullptr), socket(nullptr)
{
}

ZMQDataReceiver::~ZMQDataReceiver()
{
    stop();
}

void ZMQDataReceiver::start()
{
    if (running) return;
    running = true;
    
    try {
        context = new zmq::context_t(1);
        socket = new zmq::socket_t(*context, zmq::socket_type::sub);
        socket->set(zmq::sockopt::subscribe, "");
        socket->connect(endpoint.toStdString());
        
        emit connectionStatusChanged(true);
        receiveData();
    } catch (const std::exception &e) {
        qWarning() << "ZMQ 连接失败:" << e.what();
        emit connectionStatusChanged(false);
    }
}

void ZMQDataReceiver::stop()
{
    running = false;
    if (socket) {
        delete socket;
        socket = nullptr;
    }
    if (context) {
        delete context;
        context = nullptr;
    }
    emit connectionStatusChanged(false);
}

void ZMQDataReceiver::receiveData()
{
    // TODO: 实现数据接收逻辑
}

// ============================================================================
// RemoteDataMonitor 实现
// ============================================================================

RemoteDataMonitor::RemoteDataMonitor(QWidget *parent)
    : QWidget(parent), receiver(nullptr), receiverThread(nullptr)
{
    setupUI();
}

RemoteDataMonitor::~RemoteDataMonitor()
{
    disconnectFromServer();
}

void RemoteDataMonitor::setupUI()
{
    Ui::RemoteDataMonitorUi ui;
    ui.setupUi(this);
    dataTable = findChild<QTableWidget*>("dataTable");
    if (!dataTable) {
        return;
    }
    dataTable->setColumnCount(4);
    dataTable->setHorizontalHeaderLabels({"时间戳", "数据类型", "值", "单位"});
    dataTable->setAlternatingRowColors(true);
    dataTable->setColumnWidth(0, 150);
    dataTable->setColumnWidth(1, 100);
    dataTable->setColumnWidth(2, 100);
    dataTable->setColumnWidth(3, 80);
}

void RemoteDataMonitor::connectToServer(const QString &zmqEndpoint)
{
    receiver = new ZMQDataReceiver(zmqEndpoint);
    receiverThread = new QThread();
    receiver->moveToThread(receiverThread);
    
    connect(receiver, &ZMQDataReceiver::dataReceived, this, &RemoteDataMonitor::onDataReceived);
    connect(receiverThread, &QThread::started, receiver, &ZMQDataReceiver::start);
    
    receiverThread->start();
}

void RemoteDataMonitor::disconnectFromServer()
{
    if (receiver) {
        receiver->stop();
        receiverThread->quit();
        receiverThread->wait();
        delete receiver;
        delete receiverThread;
        receiver = nullptr;
        receiverThread = nullptr;
    }
}

void RemoteDataMonitor::onDataReceived(const QByteArray &data)
{
    qDebug() << "接收到数据:" << data.size() << "字节";
    
    // 示例：添加数据到表格
    int row = dataTable->rowCount();
    dataTable->insertRow(row);
    
    QDateTime now = QDateTime::currentDateTime();
    dataTable->setItem(row, 0, new QTableWidgetItem(now.toString("hh:mm:ss.zzz")));
    dataTable->setItem(row, 1, new QTableWidgetItem("传感器数据"));
    dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(0.0, 'f', 2)));
    dataTable->setItem(row, 3, new QTableWidgetItem("mm"));
    
    // 保持最多 100 行数据
    if (dataTable->rowCount() > 100) {
        dataTable->removeRow(0);
    }
}

// ============================================================================
// RemoteMonitorPanel 实现
// ============================================================================

RemoteMonitorPanel::RemoteMonitorPanel(QWidget *parent)
    : QWidget(parent), dataMonitor(nullptr)
{
    setupUI();
    setupConnections();
}

RemoteMonitorPanel::~RemoteMonitorPanel()
{
    stopMonitoring();
}

void RemoteMonitorPanel::setupUI()
{
    Ui::RemoteMonitorPanelUi ui;
    ui.setupUi(this);

    dataMonitor = findChild<RemoteDataMonitor*>("dataMonitor");
    connectButton = findChild<QPushButton*>("connectButton");
    disconnectButton = findChild<QPushButton*>("disconnectButton");
    statusLabel = findChild<QLabel*>("statusLabel");
}

void RemoteMonitorPanel::setupConnections()
{
    connect(connectButton, &QPushButton::clicked, this, &RemoteMonitorPanel::onConnectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &RemoteMonitorPanel::onDisconnectClicked);
}

void RemoteMonitorPanel::startMonitoring()
{
    if (!dataMonitor || !statusLabel) return;
    dataMonitor->connectToServer("tcp://localhost:5555");
    statusLabel->setText("已连接");
    statusLabel->setProperty("state", "connected");
    statusLabel->style()->unpolish(statusLabel);
    statusLabel->style()->polish(statusLabel);
    emit monitoringStarted();
}

void RemoteMonitorPanel::stopMonitoring()
{
    if (!dataMonitor || !statusLabel) return;
    dataMonitor->disconnectFromServer();
    statusLabel->setText("未连接");
    statusLabel->setProperty("state", "disconnected");
    statusLabel->style()->unpolish(statusLabel);
    statusLabel->style()->polish(statusLabel);
    emit monitoringStopped();
}

void RemoteMonitorPanel::onConnectClicked()
{
    startMonitoring();
}

void RemoteMonitorPanel::onDisconnectClicked()
{
    stopMonitoring();
}
