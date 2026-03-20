#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <zmq.hpp>
#include <memory>
#include <atomic>
#include "message.pb.h"
#include "config/zrcsConfig.h"

/**
 * @class ZMQClientWorker
 * @brief ZMQ 客户端工作线程（在后台线程运行，避免阻塞 GUI）
 * @details 支持指数退避重连策略，超过最大重试次数后停止重连并通知上层
 */
class ZMQClientWorker : public QObject {
    Q_OBJECT

public:
    explicit ZMQClientWorker(const QString& host = "localhost", int port = 5555);
    ~ZMQClientWorker();

    bool isConnected() const { return connected_; }

public slots:
    void connect();
    void disconnect();
    void sendCommand(const QString& command, const QVector<double>& args = {});
    void sendBTCommand(const QString& action, const QString& xmlData = {});

signals:
    void connected();
    void disconnected();
    void commandSent(const QString& command, bool success);
    void errorOccurred(const QString& error);
    void reconnectFailed();  // 超过最大重试次数后发出

private slots:
    void attemptReconnect();

private:
    QString host_;
    int port_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> connected_{false};

    // 重连状态
    QTimer* reconnectTimer_ = nullptr;
    int retryCount_ = 0;
    int currentIntervalMs_ = 0;

    void sendReply(const QString& message);
    void startReconnect();
    void stopReconnect();
};

/**
 * @class ZMQClient
 * @brief Qt 线程安全的 ZMQ 客户端（主线程接口）
 */
class ZMQClient : public QObject {
    Q_OBJECT

public:
    explicit ZMQClient(const QString& host = "localhost", int port = 5555, QObject* parent = nullptr);
    ~ZMQClient();

    bool isConnected() const;
    void connectToServer();
    void disconnectFromServer();

    // 便捷方法
    void moveJ(double j1, double j2, double j3, double j4, double j5, double j6);
    void moveL(double x, double y, double z, double rx, double ry, double rz);
    void moveC(double x1, double y1, double z1, double x2, double y2, double z2);
    void stop();
    void enable();
    void disable();
    void jog(int axis, int direction, double speed);
    void reset();
    void home();

    // 通用方法
    void sendCommand(const QString& command, const QVector<double>& args = {});

    // 行为树控制方法
    void loadBehaviorTree(const QString& xml);
    void startBehaviorTree();
    void stopBehaviorTree();

signals:
    void connected();
    void disconnected();
    void commandSent(const QString& command, bool success);
    void errorOccurred(const QString& error);
    void reconnectFailed();

private:
    ZMQClientWorker* worker_;
    QThread* worker_thread_;
};
