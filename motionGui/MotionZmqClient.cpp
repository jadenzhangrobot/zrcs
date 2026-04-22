#include "MotionZmqClient.h"
#include <QDebug>
#include <algorithm>

// ============================================================================
// MotionZmqClientWorker
// ============================================================================

MotionZmqClientWorker::MotionZmqClientWorker(const QString& host, int port, int timeoutMs)
    : host_(host), port_(port), timeoutMs_(timeoutMs)
{
}

MotionZmqClientWorker::~MotionZmqClientWorker()
{
    stopReconnect();
    disconnect();
}

void MotionZmqClientWorker::connect()
{
    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::req);
        socket_->set(zmq::sockopt::rcvtimeo, timeoutMs_);
        socket_->set(zmq::sockopt::linger, 0);

        QString endpoint = QString("tcp://%1:%2").arg(host_).arg(port_);
        socket_->connect(endpoint.toStdString());

        connected_ = true;
        retryCount_ = 0;
        stopReconnect();
        qDebug() << "[MotionZmqClient] Connected to" << endpoint;
        emit connected();
    } catch (const zmq::error_t& e) {
        connected_ = false;
        emit errorOccurred(QString("Connection error: %1").arg(e.what()));
        if (autoReconnect_) {
            startReconnect();
        }
    }
}

void MotionZmqClientWorker::disconnect()
{
    stopReconnect();
    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
        connected_ = false;
        qDebug() << "[MotionZmqClient] Disconnected";
        emit disconnected();
    } catch (const zmq::error_t& e) {
        qDebug() << "[MotionZmqClient] Disconnect error:" << e.what();
    }
}

QString MotionZmqClientWorker::sendRaw(const std::string& data, const QString& tag)
{
    if (!connected_) {
        emit errorOccurred("Not connected to server");
        return {};
    }

    try {
        zmq::message_t request(data.size());
        memcpy(request.data(), data.data(), data.size());
        socket_->send(request, zmq::send_flags::none);

        qDebug() << "[MotionZmqClient] Sent:" << tag << "size=" << data.size();

        zmq::message_t reply;
        auto result = socket_->recv(reply, zmq::recv_flags::none);

        if (result) {
            QString response = QString::fromUtf8(
                static_cast<const char*>(reply.data()), static_cast<int>(reply.size()));
            qDebug() << "[MotionZmqClient] Reply:" << response;
            emit replyReceived(tag, response);
            return response;
        }

        emit errorOccurred(QString("No response from server (%1)").arg(tag));
        handleCommFailure();
        return {};

    } catch (const zmq::error_t& e) {
        emit errorOccurred(QString("Send error (%1): %2").arg(tag, e.what()));
        handleCommFailure();
        return {};
    }
}

void MotionZmqClientWorker::handleCommFailure()
{
    connected_ = false;
    emit disconnected();
    if (autoReconnect_) {
        startReconnect();
    }
}

void MotionZmqClientWorker::sendCommand(const QString& command, const QVector<double>& args)
{
    zrcs_message::MotionCommand cmd;
    cmd.set_command(command.toStdString());
    for (double arg : args) {
        cmd.add_args(arg);
    }

    std::string serialized;
    if (!cmd.SerializeToString(&serialized)) {
        emit errorOccurred("Failed to serialize MotionCommand");
        return;
    }

    sendRaw(serialized, command);
}

void MotionZmqClientWorker::startReconnect()
{
    if (reconnectTimer_ && reconnectTimer_->isActive()) {
        return;
    }

    currentIntervalMs_ = reconnectIntervalMs_;
    retryCount_ = 0;

    if (!reconnectTimer_) {
        reconnectTimer_ = new QTimer(this);
        reconnectTimer_->setSingleShot(true);
        QObject::connect(reconnectTimer_, &QTimer::timeout,
                         this, &MotionZmqClientWorker::attemptReconnect);
    }

    qDebug() << "[MotionZmqClient] Starting reconnect, interval:" << currentIntervalMs_ << "ms";
    reconnectTimer_->start(currentIntervalMs_);
}

void MotionZmqClientWorker::stopReconnect()
{
    if (reconnectTimer_) {
        reconnectTimer_->stop();
    }
}

void MotionZmqClientWorker::attemptReconnect()
{
    if (maxRetries_ > 0 && retryCount_ >= maxRetries_) {
        qDebug() << "[MotionZmqClient] Max reconnect retries reached, giving up";
        emit reconnectFailed();
        return;
    }

    ++retryCount_;
    qDebug() << "[MotionZmqClient] Reconnect attempt" << retryCount_;

    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
    } catch (...) {}

    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::req);
        socket_->set(zmq::sockopt::rcvtimeo, timeoutMs_);
        socket_->set(zmq::sockopt::linger, 0);

        QString endpoint = QString("tcp://%1:%2").arg(host_).arg(port_);
        socket_->connect(endpoint.toStdString());

        connected_ = true;
        retryCount_ = 0;
        qDebug() << "[MotionZmqClient] Reconnected to" << endpoint;
        emit connected();
        return;
    } catch (const zmq::error_t& e) {
        connected_ = false;
        qDebug() << "[MotionZmqClient] Reconnect failed:" << e.what();
    }

    currentIntervalMs_ = std::min(
        currentIntervalMs_ * backoffMultiplier_,
        maxReconnectIntervalMs_);
    reconnectTimer_->start(currentIntervalMs_);
}

// ============================================================================
// MotionZmqClient
// ============================================================================

MotionZmqClient::MotionZmqClient(const QString& host, int port, int timeoutMs, QObject* parent)
    : QObject(parent), worker_(nullptr), workerThread_(nullptr)
{
    workerThread_ = new QThread(this);
    worker_ = new MotionZmqClientWorker(host, port, timeoutMs);
    worker_->moveToThread(workerThread_);

    QObject::connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    QObject::connect(this, &MotionZmqClient::destroyed, workerThread_, &QThread::quit);

    QObject::connect(worker_, &MotionZmqClientWorker::connected,       this, &MotionZmqClient::connected);
    QObject::connect(worker_, &MotionZmqClientWorker::disconnected,    this, &MotionZmqClient::disconnected);
    QObject::connect(worker_, &MotionZmqClientWorker::replyReceived,   this, &MotionZmqClient::replyReceived);
    QObject::connect(worker_, &MotionZmqClientWorker::errorOccurred,   this, &MotionZmqClient::errorOccurred);
    QObject::connect(worker_, &MotionZmqClientWorker::reconnectFailed, this, &MotionZmqClient::reconnectFailed);

    workerThread_->start();
}

MotionZmqClient::~MotionZmqClient()
{
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

bool MotionZmqClient::isConnected() const
{
    return worker_ ? worker_->isConnected() : false;
}

void MotionZmqClient::connectToServer()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "connect", Qt::QueuedConnection);
    }
}

void MotionZmqClient::disconnectFromServer()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "disconnect", Qt::QueuedConnection);
    }
}

void MotionZmqClient::sendCommand(const QString& command, const QVector<double>& args)
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(QString, command),
                                  Q_ARG(QVector<double>, args));
    }
}
