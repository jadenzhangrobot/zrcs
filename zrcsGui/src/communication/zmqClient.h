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
#include "config/zrcsConfig.h"

/**
 * @class ZMQClientWorker
 * @brief ZMQ 通信工作线程（后台线程运行，避免阻塞 GUI）
 * @details 纯通信层：ZMQ REQ/REP + Protobuf 序列化
 *          支持指数退避重连策略
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

    /** @brief 发送 MotionCommand（通用命令名 + 参数列表） */
    void sendCommand(const QString& command, const QVector<double>& args = {});

    /** @brief 发送 BehaviorTreeCommand（action + 可选 XML） */
    void sendBTCommand(const QString& action, const QString& xmlData = {});

signals:
    void connected();
    void disconnected();
    void replyReceived(const QString& command, const QString& reply);
    void errorOccurred(const QString& error);
    void reconnectFailed();

private slots:
    void attemptReconnect();

private:
    QString host_;
    int port_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> connected_{false};

    QTimer* reconnectTimer_ = nullptr;
    int retryCount_ = 0;
    int currentIntervalMs_ = 0;

    /**
     * @brief 底层发送/接收：序列化后的二进制 → ZMQ REQ → 等待 REP
     * @param data 已序列化的 Protobuf 二进制
     * @param tag  标识符，用于日志和回复信号
     * @return 回复字符串；通信失败返回空并触发重连
     */
    QString sendRaw(const std::string& data, const QString& tag);

    void startReconnect();
    void stopReconnect();
    void handleCommFailure();
};

/**
 * @class ZMQClient
 * @brief 线程安全的 ZMQ 客户端（GUI 主线程接口）
 * @details 纯通信层封装，不含任何业务逻辑
 *          业务方通过 sendCommand / sendBTCommand 发送命令
 */
class ZMQClient : public QObject {
    Q_OBJECT

public:
    explicit ZMQClient(const QString& host = "localhost", int port = 5555, QObject* parent = nullptr);
    ~ZMQClient();

    bool isConnected() const;
    void connectToServer();
    void disconnectFromServer();

    /** @brief 发送通用运动命令（MotionCommand protobuf） */
    void sendCommand(const QString& command, const QVector<double>& args = {});

    /** @brief 发送行为树命令（TypedCommand + BehaviorTreeCommand protobuf） */
    void sendBTCommand(const QString& action, const QString& xmlData = {});

signals:
    void connected();
    void disconnected();
    void replyReceived(const QString& command, const QString& reply);
    void errorOccurred(const QString& error);
    void reconnectFailed();

private:
    ZMQClientWorker* worker_;
    QThread* worker_thread_;
};
