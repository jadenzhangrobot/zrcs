#include "zmqClient.h"
#include <QDebug>
#include <QThread>
#include <algorithm>

// ============================================================================
// ZMQClientWorker 实现
// ============================================================================

ZMQClientWorker::ZMQClientWorker(const QString& host, int port)
    : host_(host), port_(port), context_(nullptr), socket_(nullptr)
{
}

ZMQClientWorker::~ZMQClientWorker()
{
    stopReconnect();
    disconnect();
}

void ZMQClientWorker::connect()
{
    try {
        context_ = std::make_unique<zmq::context_t>(1);
        socket_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::req);
        socket_->set(zmq::sockopt::rcvtimeo, 5000); // 5s timeout
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
        QString error = QString("[ZMQClient] Connection error: %1").arg(e.what());
        qDebug() << error;
        emit errorOccurred(error);

        // 连接失败，启动重连
        const auto& commCfg = ZrcsConfig::Config::instance().comm;
        if (commCfg.zmqAutoReconnect) {
            startReconnect();
        }
    }
}

void ZMQClientWorker::disconnect()
{
    stopReconnect();
    try {
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        if (context_) {
            context_.reset();
        }
        connected_ = false;
        qDebug() << "[ZMQClient] Disconnected";
        emit disconnected();
    } catch (const zmq::error_t& e) {
        qDebug() << "[ZMQClient] Disconnect error:" << e.what();
    }
}

void ZMQClientWorker::sendCommand(const QString& command, const QVector<double>& args)
{
    if (!connected_) {
        emit errorOccurred("Not connected to server");
        return;
    }

    try {
        // 创建 Protobuf 消息
        zrcs_message::MotionCommand cmd;
        cmd.set_command(command.toStdString());
        for (double arg : args) {
            cmd.add_args(arg);
        }

        // 序列化
        std::string serialized;
        if (!cmd.SerializeToString(&serialized)) {
            emit errorOccurred("Failed to serialize command");
            return;
        }

        // 发送
        zmq::message_t request(serialized.size());
        memcpy(request.data(), serialized.data(), serialized.size());
        socket_->send(request, zmq::send_flags::none);

        qDebug() << "[ZMQClient] Sent:" << command << "with" << args.size() << "args";

        // 接收回复
        zmq::message_t reply;
        auto result = socket_->recv(reply, zmq::recv_flags::none);

        if (result) {
            std::string response(static_cast<char*>(reply.data()), reply.size());
            bool success = (response == "OK");
            qDebug() << "[ZMQClient] Response:" << QString::fromStdString(response);
            emit commandSent(command, success);
        } else {
            emit errorOccurred("No response from server");
            // 通信超时，可能服务端已断开
            connected_ = false;
            emit disconnected();
            const auto& commCfg = ZrcsConfig::Config::instance().comm;
            if (commCfg.zmqAutoReconnect) {
                startReconnect();
            }
        }

    } catch (const zmq::error_t& e) {
        QString error = QString("[ZMQClient] Send error: %1").arg(e.what());
        qDebug() << error;
        emit errorOccurred(error);
        // 通信异常，标记断开并尝试重连
        connected_ = false;
        emit disconnected();
        const auto& commCfg = ZrcsConfig::Config::instance().comm;
        if (commCfg.zmqAutoReconnect) {
            startReconnect();
        }
    }
}

void ZMQClientWorker::startReconnect()
{
    if (reconnectTimer_ && reconnectTimer_->isActive()) {
        return; // 已在重连中
    }

    const auto& commCfg = ZrcsConfig::Config::instance().comm;
    currentIntervalMs_ = commCfg.zmqReconnectIntervalMs;
    retryCount_ = 0;

    if (!reconnectTimer_) {
        reconnectTimer_ = new QTimer(this);
        reconnectTimer_->setSingleShot(true);
        QObject::connect(reconnectTimer_, &QTimer::timeout, this, &ZMQClientWorker::attemptReconnect);
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

    // 检查是否超过最大重试次数
    if (commCfg.zmqMaxRetries > 0 && retryCount_ >= commCfg.zmqMaxRetries) {
        qDebug() << "[ZMQClient] Max reconnect retries reached (" << commCfg.zmqMaxRetries << "), giving up";
        emit reconnectFailed();
        return;
    }

    ++retryCount_;
    qDebug() << "[ZMQClient] Reconnect attempt" << retryCount_
             << "/" << (commCfg.zmqMaxRetries > 0 ? QString::number(commCfg.zmqMaxRetries) : "inf");

    // 先清理旧连接
    try {
        if (socket_) { socket_->close(); socket_.reset(); }
        if (context_) { context_.reset(); }
    } catch (...) {}

    // 尝试连接
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

    // 指数退避：interval *= multiplier，但不超过上限
    currentIntervalMs_ = std::min(
        currentIntervalMs_ * commCfg.zmqBackoffMultiplier,
        commCfg.zmqMaxReconnectIntervalMs
    );
    reconnectTimer_->start(currentIntervalMs_);
}

// ============================================================================
// ZMQClient 实现
// ============================================================================

ZMQClient::ZMQClient(const QString& host, int port, QObject* parent)
    : QObject(parent), worker_(nullptr), worker_thread_(nullptr)
{
    // 创建工作线程
    worker_thread_ = new QThread(this);
    worker_ = new ZMQClientWorker(host, port);
    worker_->moveToThread(worker_thread_);

    // 连接信号槽
    QObject::connect(worker_thread_, &QThread::finished, worker_, &QObject::deleteLater);
    QObject::connect(this, &ZMQClient::destroyed, worker_thread_, &QThread::quit);

    // 转发信号
    QObject::connect(worker_, &ZMQClientWorker::connected, this, &ZMQClient::connected);
    QObject::connect(worker_, &ZMQClientWorker::disconnected, this, &ZMQClient::disconnected);
    QObject::connect(worker_, &ZMQClientWorker::commandSent, this, &ZMQClient::commandSent);
    QObject::connect(worker_, &ZMQClientWorker::errorOccurred, this, &ZMQClient::errorOccurred);
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

void ZMQClient::moveJ(double j1, double j2, double j3, double j4, double j5, double j6)
{
    sendCommand("MoveJ", {j1, j2, j3, j4, j5, j6});
}

void ZMQClient::moveL(double x, double y, double z, double rx, double ry, double rz)
{
    sendCommand("MoveL", {x, y, z, rx, ry, rz});
}

void ZMQClient::moveC(double x1, double y1, double z1, double x2, double y2, double z2)
{
    sendCommand("MoveC", {x1, y1, z1, x2, y2, z2});
}

void ZMQClient::stop()
{
    sendCommand("Stop");
}

void ZMQClient::enable()
{
    sendCommand("Enable");
}

void ZMQClient::disable()
{
    sendCommand("Disable");
}

void ZMQClient::jog(int axis, int direction, double speed)
{
    sendCommand("Jog", {static_cast<double>(axis), static_cast<double>(direction), speed});
}

void ZMQClient::reset()
{
    sendCommand("Reset");
}

void ZMQClient::home()
{
    sendCommand("Home");
}

void ZMQClient::sendCommand(const QString& command, const QVector<double>& args)
{
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "sendCommand", Qt::QueuedConnection,
                                Q_ARG(QString, command),
                                Q_ARG(QVector<double>, args));
    }
}
