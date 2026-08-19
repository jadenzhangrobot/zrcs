#include "ZmqStatusSubscriber.h"
#include <QDebug>
#include <QDateTime>
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

            for (int f = 0; f < status.axis_feedback_frames_size(); ++f) {
                const auto& frame = status.axis_feedback_frames(f);
                QVector<double> positions;
                QVector<quint8> servoStates;
                positions.reserve(frame.axes_size());
                servoStates.reserve(frame.axes_size());
                for (int i = 0; i < frame.axes_size(); ++i) {
                    positions.append(frame.axes(i).position());
                    servoStates.append(frame.axes(i).servo_enabled() ? 1 : 0);
                }
                emit axisPositionsUpdated(positions);
                emit axisServoStatesUpdated(servoStates);
            }

            if (status.heartbeat() > 0) {
                emit heartbeatReceived(status.heartbeat());
            }

            if (!status.system_state().empty()) {
                emit taskSchedulingUpdated(QString::fromStdString(status.system_state()));
            }

            for (int i = 0; i < status.rt_logs_size(); ++i) {
                const auto& log = status.rt_logs(i);
                const QString message = QString::fromUtf8(log.message().c_str());
                const QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
                emit rtLogReceived(log.level(), message, timestamp);
            }

            if (status.has_bt_status()) {
                const auto& bt = status.bt_status();
                emit btStatusUpdated(QString::fromStdString(bt.tree_state()),
                                     QString::fromStdString(bt.current_node()),
                                     QString::fromStdString(bt.message()));
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
    connect(worker_, &ZMQStatusWorker::axisServoStatesUpdated,
            this, &ZMQStatusSubscriber::axisServoStatesUpdated);
    connect(worker_, &ZMQStatusWorker::heartbeatReceived,
            this, &ZMQStatusSubscriber::heartbeatReceived);
    connect(worker_, &ZMQStatusWorker::taskSchedulingUpdated,
            this, &ZMQStatusSubscriber::taskSchedulingUpdated);
    connect(worker_, &ZMQStatusWorker::rtLogReceived,
            this, &ZMQStatusSubscriber::rtLogReceived);
    connect(worker_, &ZMQStatusWorker::btStatusUpdated,
            this, &ZMQStatusSubscriber::btStatusUpdated);
    connect(worker_, &ZMQStatusWorker::errorOccurred,
            this, &ZMQStatusSubscriber::errorOccurred);

    connect(workerThread_, &QThread::started, worker_, &ZMQStatusWorker::start);
}

ZMQStatusSubscriber::~ZMQStatusSubscriber()
{
    stop();
}

void ZMQStatusSubscriber::start()
{
    if (workerThread_ && !workerThread_->isRunning()) {
        workerThread_->start();
    }
}

void ZMQStatusSubscriber::stop()
{
    if (worker_) {
        worker_->stop();
    }
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait(1000);
    }
}
