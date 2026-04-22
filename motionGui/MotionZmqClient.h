#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <zmq.hpp>
#include <memory>
#include <atomic>
#include "message.pb.h"

class MotionZmqClientWorker : public QObject {
    Q_OBJECT

public:
    explicit MotionZmqClientWorker(const QString& host, int port, int timeoutMs = 5000);
    ~MotionZmqClientWorker();

    bool isConnected() const { return connected_; }

public slots:
    void connect();
    void disconnect();
    void sendCommand(const QString& command, const QVector<double>& args = {});

signals:
    void connected();
    void disconnected();
    void replyReceived(const QString& command, const QString& reply);
    void errorOccurred(const QString& error);
    void reconnectFailed();

private slots:
    void attemptReconnect();

private:
    QString sendRaw(const std::string& data, const QString& tag);
    void startReconnect();
    void stopReconnect();
    void handleCommFailure();

    QString host_;
    int port_;
    int timeoutMs_;
    int maxRetries_ = 10;
    int reconnectIntervalMs_ = 1000;
    int backoffMultiplier_ = 2;
    int maxReconnectIntervalMs_ = 30000;
    bool autoReconnect_ = true;

    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> connected_{false};

    QTimer* reconnectTimer_ = nullptr;
    int retryCount_ = 0;
    int currentIntervalMs_ = 0;
};

class MotionZmqClient : public QObject {
    Q_OBJECT

public:
    explicit MotionZmqClient(const QString& host = "localhost",
                              int port = 5555,
                              int timeoutMs = 5000,
                              QObject* parent = nullptr);
    ~MotionZmqClient();

    bool isConnected() const;
    void connectToServer();
    void disconnectFromServer();
    void sendCommand(const QString& command, const QVector<double>& args = {});

signals:
    void connected();
    void disconnected();
    void replyReceived(const QString& command, const QString& reply);
    void errorOccurred(const QString& error);
    void reconnectFailed();

private:
    MotionZmqClientWorker* worker_;
    QThread* workerThread_;
};
