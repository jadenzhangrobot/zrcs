#pragma once

#include <QWidget>
#include <QThread>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QTabWidget>
#include <zmq.hpp>
#include <google/protobuf/message.h>

/**
 * ZMQ 数据接收线程
 * 在后台线程中接收 ZMQ 数据，避免阻塞 GUI
 */
class ZMQDataReceiver : public QObject {
    Q_OBJECT

public:
    explicit ZMQDataReceiver(const QString &endpoint);
    ~ZMQDataReceiver();

    void start();
    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void connectionStatusChanged(bool connected);

private:
    void receiveData();

    QString endpoint;
    bool running;
    zmq::context_t *context;
    zmq::socket_t *socket;
};

/**
 * 远程数据监控
 */
class RemoteDataMonitor : public QWidget {
    Q_OBJECT

public:
    explicit RemoteDataMonitor(QWidget *parent = nullptr);
    ~RemoteDataMonitor();

    void connectToServer(const QString &zmqEndpoint);
    void disconnectFromServer();

signals:
    void dataUpdated(const QByteArray &data);

private slots:
    void onDataReceived(const QByteArray &data);

private:
    void setupUI();

    QTableWidget *dataTable;
    ZMQDataReceiver *receiver;
    QThread *receiverThread;
};

/**
 * 远程监控面板
 */
class RemoteMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit RemoteMonitorPanel(QWidget *parent = nullptr);
    ~RemoteMonitorPanel();

    void startMonitoring();
    void stopMonitoring();

signals:
    void monitoringStarted();
    void monitoringStopped();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();

private:
    void setupUI();
    void setupConnections();

    RemoteDataMonitor *dataMonitor;
    
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QLabel *statusLabel;
};

