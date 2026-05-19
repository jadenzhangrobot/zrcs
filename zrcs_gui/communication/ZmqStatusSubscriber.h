#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <zmq.hpp>
#include <memory>
#include <atomic>
#include "message.pb.h"

/**
 * @class ZMQStatusWorker
 * @brief ZMQ SUB 后台工作线程，接收 NRT 发布的轴状态数据
 */
class ZMQStatusWorker : public QObject {
    Q_OBJECT

public:
    explicit ZMQStatusWorker(const QString& host, int port = 5556);
    ~ZMQStatusWorker();

public slots:
    void start();
    void stop();

signals:
    void axisPositionsUpdated(QVector<double> positions);
    void heartbeatReceived(quint64 heartbeat);
    void taskSchedulingUpdated(QString state);
    void rtLogReceived(quint32 level, QString message, QString timestamp);
    void errorOccurred(const QString& error);

private:
    void pollLoop();

    QString host_;
    int port_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_{false};
};

/**
 * @class ZMQStatusSubscriber
 * @brief GUI 主线程接口，封装后台 SUB worker
 */
class ZMQStatusSubscriber : public QObject {
    Q_OBJECT

public:
    explicit ZMQStatusSubscriber(const QString& host = "localhost",
                                  int port = 5556,
                                  QObject* parent = nullptr);
    ~ZMQStatusSubscriber();

    void start();
    void stop();

signals:
    void axisPositionsUpdated(QVector<double> positions);
    void heartbeatReceived(quint64 heartbeat);
    void taskSchedulingUpdated(QString state);
    void rtLogReceived(quint32 level, QString message, QString timestamp);
    void errorOccurred(const QString& error);

private:
    ZMQStatusWorker* worker_;
    QThread* workerThread_;
};
