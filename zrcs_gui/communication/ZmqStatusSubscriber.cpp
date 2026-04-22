#include "ZmqStatusSubscriber.h"
#include <QDebug>
#include <QTimer>

// ============================================================================
// ZMQStatusWorker
// ============================================================================

ZMQStatusWorker::ZMQStatusWorker(const QString& host, int port)
    : host_(host), port_(port)
{
}

ZMQStatusWorker::~ZMQStatusWorker()
{
    stop();
}

void ZMQStatusWorker::start()
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
        qDebug() << "[ZMQStatusSubscriber] Connected to" << endpoint;
    } catch (const zmq::error_t& e) {
        running_ = false;
        emit errorOccurred(QString("SUB connect error: %1").arg(e.what()));
        return;
    }

    pollLoop();
}

void ZMQStatusWorker::stop()
{
    running_ = false;
}

void ZMQStatusWorker::pollLoop()
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

            QVector<double> positions;
            positions.reserve(status.axes_size());
            for (int i = 0; i < status.axes_size(); ++i) {
                positions.append(status.axes(i).position());
            }

            emit axisPositionsUpdated(positions);

            if (status.heartbeat() > 0) {
                emit heartbeatReceived(status.heartbeat());
            }

        } catch (const zmq::error_t& e) {
            if (running_ && e.num() != EAGAIN) {
                emit errorOccurred(QString("SUB recv error: %1").arg(e.what()));
            }
        }
    }

    // cleanup
    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
    } catch (...) {}
}

// ============================================================================
// ZMQStatusSubscriber
// ============================================================================

ZMQStatusSubscriber::ZMQStatusSubscriber(const QString& host, int port, QObject* parent)
    : QObject(parent), worker_(nullptr), workerThread_(nullptr)
{
    workerThread_ = new QThread(this);
    worker_ = new ZMQStatusWorker(host, port);
    worker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    // forward signals
    connect(worker_, &ZMQStatusWorker::axisPositionsUpdated,
            this, &ZMQStatusSubscriber::axisPositionsUpdated);
    connect(worker_, &ZMQStatusWorker::heartbeatReceived,
            this, &ZMQStatusSubscriber::heartbeatReceived);
    connect(worker_, &ZMQStatusWorker::errorOccurred,
            this, &ZMQStatusSubscriber::errorOccurred);

    workerThread_->start();
}

ZMQStatusSubscriber::~ZMQStatusSubscriber()
{
    stop();
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void ZMQStatusSubscriber::start()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "start", Qt::QueuedConnection);
    }
}

void ZMQStatusSubscriber::stop()
{
    if (worker_) {
        worker_->stop();
    }
}
