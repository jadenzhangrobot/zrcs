#include "MotionStatusSubscriber.h"
#include <QDebug>

// ============================================================================
// MotionStatusWorker
// ============================================================================

MotionStatusWorker::MotionStatusWorker(const QString& host, int port)
    : host_(host), port_(port)
{
}

MotionStatusWorker::~MotionStatusWorker()
{
    stop();
}

void MotionStatusWorker::start()
{
    if (running_.exchange(true)) return;

    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::sub);
        socket_->set(zmq::sockopt::rcvtimeo, 100);
        socket_->set(zmq::sockopt::linger, 0);
        socket_->set(zmq::sockopt::subscribe, "");

        QString endpoint = QString("tcp://%1:%2").arg(host_).arg(port_);
        socket_->connect(endpoint.toStdString());
        qDebug() << "[MotionStatusSubscriber] Connected to" << endpoint;
    } catch (const zmq::error_t& e) {
        running_ = false;
        emit errorOccurred(QString("SUB connect error: %1").arg(e.what()));
        return;
    }

    pollLoop();
}

void MotionStatusWorker::stop()
{
    running_ = false;
}

void MotionStatusWorker::pollLoop()
{
    while (running_) {
        try {
            zmq::message_t msg;
            auto result = socket_->recv(msg, zmq::recv_flags::none);
            if (!result) continue;

            zrcs_message::SystemStatus status;
            if (!status.ParseFromArray(msg.data(), static_cast<int>(msg.size()))) {
                continue;
            }

            QVector<AxisStatusData> axes;
            axes.reserve(status.axes_size());
            for (int i = 0; i < status.axes_size(); ++i) {
                const auto& ax = status.axes(i);
                AxisStatusData d;
                d.axisId = i;
                d.position = ax.position();
                d.cmdPosition = ax.cmd_position();
                d.velocity = ax.velocity();
                d.torque = ax.torque();
                axes.append(d);
            }

            emit statusUpdated(axes, status.heartbeat());

        } catch (const zmq::error_t& e) {
            if (running_ && e.num() != EAGAIN) {
                emit errorOccurred(QString("SUB recv error: %1").arg(e.what()));
            }
        }
    }

    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
    } catch (...) {}
}

// ============================================================================
// MotionStatusSubscriber
// ============================================================================

MotionStatusSubscriber::MotionStatusSubscriber(const QString& host, int port, QObject* parent)
    : QObject(parent), worker_(nullptr), workerThread_(nullptr)
{
    workerThread_ = new QThread(this);
    worker_ = new MotionStatusWorker(host, port);
    worker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    connect(worker_, &MotionStatusWorker::statusUpdated,
            this, &MotionStatusSubscriber::statusUpdated);
    connect(worker_, &MotionStatusWorker::errorOccurred,
            this, &MotionStatusSubscriber::errorOccurred);

    workerThread_->start();
}

MotionStatusSubscriber::~MotionStatusSubscriber()
{
    stop();
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void MotionStatusSubscriber::start()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "start", Qt::QueuedConnection);
    }
}

void MotionStatusSubscriber::stop()
{
    if (worker_) {
        worker_->stop();
    }
}
