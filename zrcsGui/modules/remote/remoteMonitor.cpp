#include "remote/remoteMonitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QDateTime>
#include <QTableWidgetItem>

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
        socket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
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
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel("远程数据监控");
    titleLabel->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);
    
    dataTable = new QTableWidget();
    dataTable->setColumnCount(4);
    dataTable->setHorizontalHeaderLabels({"时间戳", "数据类型", "值", "单位"});
    dataTable->setStyleSheet(
        "QTableWidget { background-color: #1a1a1a; color: #00FF00; }"
        "QHeaderView::section { background-color: #2a2a2a; color: #00FF00; padding: 5px; }"
        "QTableWidget::item { padding: 5px; }"
    );
    dataTable->setAlternatingRowColors(true);
    dataTable->setColumnWidth(0, 150);
    dataTable->setColumnWidth(1, 100);
    dataTable->setColumnWidth(2, 100);
    dataTable->setColumnWidth(3, 80);
    
    layout->addWidget(dataTable);
    
    setStyleSheet("background-color: #1a1a1a;");
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
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 标题
    QLabel *titleLabel = new QLabel("远程监控面板");
    titleLabel->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 16px;");
    mainLayout->addWidget(titleLabel);
    
    // 数据监控
    dataMonitor = new RemoteDataMonitor();
    mainLayout->addWidget(dataMonitor);
    
    // 控制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    connectButton = new QPushButton("连接");
    disconnectButton = new QPushButton("断开");
    statusLabel = new QLabel("未连接");
    
    connectButton->setStyleSheet(
        "QPushButton { background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 8px; }"
        "QPushButton:hover { background-color: #3a7a3a; }"
    );
    disconnectButton->setStyleSheet(
        "QPushButton { background-color: #5a2a2a; color: #FF6347; border: 1px solid #FF6347; padding: 8px; }"
        "QPushButton:hover { background-color: #7a3a3a; }"
    );
    statusLabel->setStyleSheet("color: #FF6347; font-weight: bold;");
    
    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(disconnectButton);
    buttonLayout->addWidget(statusLabel);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    setStyleSheet("background-color: #1a1a1a;");
}

void RemoteMonitorPanel::setupConnections()
{
    connect(connectButton, &QPushButton::clicked, this, &RemoteMonitorPanel::onConnectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &RemoteMonitorPanel::onDisconnectClicked);
}

void RemoteMonitorPanel::startMonitoring()
{
    dataMonitor->connectToServer("tcp://localhost:5555");
    statusLabel->setText("已连接");
    statusLabel->setStyleSheet("color: #00FF00; font-weight: bold;");
    emit monitoringStarted();
}

void RemoteMonitorPanel::stopMonitoring()
{
    dataMonitor->disconnectFromServer();
    statusLabel->setText("未连接");
    statusLabel->setStyleSheet("color: #FF6347; font-weight: bold;");
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
