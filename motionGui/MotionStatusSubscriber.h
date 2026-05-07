#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <zmq.hpp>
#include <memory>
#include <atomic>
#include "message.pb.h"

struct AxisStatusData {
    int axisId = 0;
    double position = 0.0;
    double cmdPosition = 0.0;
    double plannerVelocity = 0.0;
    double velocity = 0.0;
    double torque = 0.0;
};

class MotionStatusWorker : public QObject {
    Q_OBJECT

public:
    explicit MotionStatusWorker(const QString& host, int port = 5556);
    ~MotionStatusWorker();

public slots:
    void start();
    void stop();

signals:
    void statusUpdated(const QVector<AxisStatusData>& axes, quint64 heartbeat);
    void errorOccurred(const QString& error);

private:
    void pollLoop();

    QString host_;
    int port_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_{false};
};

class MotionStatusSubscriber : public QObject {
    Q_OBJECT

public:
    explicit MotionStatusSubscriber(const QString& host = "localhost",
                                     int port = 5556,
                                     QObject* parent = nullptr);
    ~MotionStatusSubscriber();

    void start();
    void stop();

signals:
    void statusUpdated(const QVector<AxisStatusData>& axes, quint64 heartbeat);
    void errorOccurred(const QString& error);

private:
    MotionStatusWorker* worker_;
    QThread* workerThread_;
};
