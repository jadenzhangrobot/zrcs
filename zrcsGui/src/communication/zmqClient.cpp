#include "zmqClient.h"
#include <QDebug>

// ============================================================================
// ZMQClientWorker
// ============================================================================

ZMQClientWorker::ZMQClientWorker(const QString& host, int port)
    : host_(host), port_(port)
{
}

ZMQClientWorker::~ZMQClientWorker()
{
    stopReconnect();
    disconnect();
}

void ZMQClientWorker::connect()
{
    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::req);
        socket_->set(zmq::sockopt::rcvtimeo, commCfg.zmqTimeoutMs);
        socket_->set(zmq::sockopt::linger, 0);

        QString endpoint = QString("tcp://%1:%2").arg(host_).arg(port_);
        socket_->connect(endpoint.toStdString());

        connected_ = true;
        retryCount_ = 0;
        stopReconnect();
        qDebug() << "[ZMQClient] Connected to" << endpoint;
        emit connected();
    } catch (const zmq::error_t& e) {
        connected_ = false;
        emit errorOccurred(QString("Connection error: %1").arg(e.what()));
        if (commCfg.zmqAutoReconnect) {
            startReconnect();
        }
    }
}

void ZMQClientWorker::disconnect()
{
    stopReconnect();
    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
        connected_ = false;
        qDebug() << "[ZMQClient] Disconnected";
        emit disconnected();
    } catch (const zmq::error_t& e) {
        qDebug() << "[ZMQClient] Disconnect error:" << e.what();
    }
}

// ---------------------------------------------------------------------------
// 核心发送：序列化 protobuf → ZMQ REQ → 等 REP
// ---------------------------------------------------------------------------

QString ZMQClientWorker::sendRaw(const std::string& data, const QString& tag)
{
    if (!connected_) {
        emit errorOccurred("Not connected to server");
        return {};
    }

    try {
        zmq::message_t request(data.size());
        memcpy(request.data(), data.data(), data.size());
        socket_->send(request, zmq::send_flags::none);

        qDebug() << "[ZMQClient] Sent:" << tag << "size=" << data.size();

        zmq::message_t reply;
        auto result = socket_->recv(reply, zmq::recv_flags::none);

        if (result) {
            QString response = QString::fromUtf8(
                static_cast<const char*>(reply.data()), static_cast<int>(reply.size()));
            qDebug() << "[ZMQClient] Reply:" << response;
            emit replyReceived(tag, response);
            return response;
        }

        // 超时，无回复
        emit errorOccurred(QString("No response from server (%1)").arg(tag));
        handleCommFailure();
        return {};

    } catch (const zmq::error_t& e) {
        emit errorOccurred(QString("Send error (%1): %2").arg(tag, e.what()));
        handleCommFailure();
        return {};
    }
}

void ZMQClientWorker::handleCommFailure()
{
    connected_ = false;
    emit disconnected();
    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    if (commCfg.zmqAutoReconnect) {
        startReconnect();
    }
}

// ---------------------------------------------------------------------------
// Protobuf 序列化 + 发送
// ---------------------------------------------------------------------------

void ZMQClientWorker::sendCommand(const QString& command, const QVector<double>& args)
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

void ZMQClientWorker::sendBTCommand(const QString& action, const QString& xmlData)
{
    zrcs_message::TypedCommand typed_cmd;
    auto* bt_cmd = typed_cmd.mutable_bt_command();
    bt_cmd->set_action(action.toStdString());
    if (!xmlData.isEmpty()) {
        bt_cmd->set_xml_data(xmlData.toStdString());
    }

    std::string serialized;
    if (!typed_cmd.SerializeToString(&serialized)) {
        emit errorOccurred("Failed to serialize BT command");
        return;
    }

    sendRaw(serialized, QString("BT_%1").arg(action));
}

// ---------------------------------------------------------------------------
// 重连
// ---------------------------------------------------------------------------

void ZMQClientWorker::startReconnect()
{
    if (reconnectTimer_ && reconnectTimer_->isActive()) {
        return;
    }

    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    currentIntervalMs_ = commCfg.zmqReconnectIntervalMs;
    retryCount_ = 0;

    if (!reconnectTimer_) {
        reconnectTimer_ = new QTimer(this);
        reconnectTimer_->setSingleShot(true);
        QObject::connect(reconnectTimer_, &QTimer::timeout,
                         this, &ZMQClientWorker::attemptReconnect);
    }

    qDebug() << "[ZMQClient] Starting reconnect, interval:" << currentIntervalMs_ << "ms";
    reconnectTimer_->start(currentIntervalMs_);
}

void ZMQClientWorker::stopReconnect()
{
    if (reconnectTimer_) {
        reconnectTimer_->stop();
    }
}

void ZMQClientWorker::attemptReconnect()
{
    const auto& commCfg = ZrcsConfig::Config::instance().comm;

    if (commCfg.zmqMaxRetries > 0 && retryCount_ >= commCfg.zmqMaxRetries) {
        qDebug() << "[ZMQClient] Max reconnect retries reached, giving up";
        emit reconnectFailed();
        return;
    }

    ++retryCount_;
    qDebug() << "[ZMQClient] Reconnect attempt" << retryCount_;

    // 清理旧连接
    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
    } catch (...) {}

    // 重新连接
    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::req);
        socket_->set(zmq::sockopt::rcvtimeo, commCfg.zmqTimeoutMs);
        socket_->set(zmq::sockopt::linger, 0);

        QString endpoint = QString("tcp://%1:%2").arg(host_).arg(port_);
        socket_->connect(endpoint.toStdString());

        connected_ = true;
        retryCount_ = 0;
        qDebug() << "[ZMQClient] Reconnected to" << endpoint;
        emit connected();
        return;
    } catch (const zmq::error_t& e) {
        connected_ = false;
        qDebug() << "[ZMQClient] Reconnect failed:" << e.what();
    }

    // 指数退避
    currentIntervalMs_ = std::min(
        currentIntervalMs_ * commCfg.zmqBackoffMultiplier,
        commCfg.zmqMaxReconnectIntervalMs);
    reconnectTimer_->start(currentIntervalMs_);
}

// ============================================================================
// ZMQClient（主线程接口）
// ============================================================================

ZMQClient::ZMQClient(const QString& host, int port, QObject* parent)
    : QObject(parent), worker_(nullptr), worker_thread_(nullptr)
{
    worker_thread_ = new QThread(this);
    worker_ = new ZMQClientWorker(host, port);
    worker_->moveToThread(worker_thread_);

    QObject::connect(worker_thread_, &QThread::finished, worker_, &QObject::deleteLater);
    QObject::connect(this, &ZMQClient::destroyed, worker_thread_, &QThread::quit);

    // 转发 Worker 信号
    QObject::connect(worker_, &ZMQClientWorker::connected,       this, &ZMQClient::connected);
    QObject::connect(worker_, &ZMQClientWorker::disconnected,    this, &ZMQClient::disconnected);
    QObject::connect(worker_, &ZMQClientWorker::replyReceived,   this, &ZMQClient::replyReceived);
    QObject::connect(worker_, &ZMQClientWorker::errorOccurred,   this, &ZMQClient::errorOccurred);
    QObject::connect(worker_, &ZMQClientWorker::reconnectFailed, this, &ZMQClient::reconnectFailed);

    worker_thread_->start();
}

ZMQClient::~ZMQClient()
{
    if (worker_thread_) {
        worker_thread_->quit();
        worker_thread_->wait();
    }
}

bool ZMQClient::isConnected() const
{
    return worker_ ? worker_->isConnected() : false;
}

void ZMQClient::connectToServer()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "connect", Qt::QueuedConnection);
    }
}

void ZMQClient::disconnectFromServer()
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "disconnect", Qt::QueuedConnection);
    }
}

void ZMQClient::sendCommand(const QString& command, const QVector<double>& args)
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(QString, command),
                                  Q_ARG(QVector<double>, args));
    }
}

void ZMQClient::sendBTCommand(const QString& action, const QString& xmlData)
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "sendBTCommand", Qt::QueuedConnection,
                                  Q_ARG(QString, action),
                                  Q_ARG(QString, xmlData));
    }
}
