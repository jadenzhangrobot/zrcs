#include "mujoco/MujocoFrameServer.h"

#include "communication/ZmqStatusSubscriber.h"
#include "mujoco/MujocoVisualizer.h"

#include "message.pb.h"

#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <zmq.hpp>

#include <cmath>
#include <string>
#include <utility>

namespace {
constexpr int kMaxRequestBytes = 64 * 1024;

int requestContentLength(const QByteArray &request, int headerEnd)
{
    const QByteArray headers = request.left(headerEnd);
    const QList<QByteArray> lines = headers.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.left(15).toLower() == QByteArrayLiteral("content-length:")) {
            bool ok = false;
            const int length = trimmed.mid(15).trimmed().toInt(&ok);
            return ok && length >= 0 ? length : -1;
        }
    }
    return 0;
}

QByteArray jsonError(const QString &message)
{
    QJsonObject object;
    object.insert(QStringLiteral("ok"), false);
    object.insert(QStringLiteral("error"), message);
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}
} // namespace

MujocoFrameServer::MujocoFrameServer(QObject *parent)
    : QObject(parent)
{
    connect(&server_, &QTcpServer::newConnection, this, &MujocoFrameServer::handleConnection);
}

MujocoFrameServer::~MujocoFrameServer()
{
    stop();
}

bool MujocoFrameServer::start(MujocoVisualizer3D *view,
                              quint16 port,
                              const QString &motionHost,
                              quint16 motionPort)
{
    stop();
    view_ = view;
    setMotionEndpoint(motionHost, motionPort);
    if (!server_.listen(QHostAddress::LocalHost, port)) {
        qWarning("[MujocoFrameServer] Failed to listen on localhost:%u: %s",
                 static_cast<unsigned>(port),
                 qPrintable(server_.errorString()));
        return false;
    }

    qInfo("[MujocoFrameServer] Listening on http://127.0.0.1:%u",
          static_cast<unsigned>(server_.serverPort()));
    return true;
}

void MujocoFrameServer::stop()
{
    if (server_.isListening()) {
        server_.close();
    }
    setStatusSubscriber(nullptr);
    view_.clear();
}

bool MujocoFrameServer::isListening() const
{
    return server_.isListening();
}

quint16 MujocoFrameServer::port() const
{
    return server_.serverPort();
}

void MujocoFrameServer::setMotionEndpoint(const QString &host, quint16 port)
{
    motionHost_ = host.trimmed().isEmpty() ? QStringLiteral("127.0.0.1") : host.trimmed();
    motionPort_ = port == 0 ? static_cast<quint16>(5555) : port;
}

void MujocoFrameServer::setStatusSubscriber(ZMQStatusSubscriber *subscriber)
{
    if (statusSubscriber_ == subscriber) {
        return;
    }

    if (statusSubscriber_) {
        disconnect(statusSubscriber_, nullptr, this, nullptr);
    }

    statusSubscriber_ = subscriber;
    axisPositions_.clear();
    axisVelocities_.clear();
    axisCommandPositions_.clear();
    servoStates_.clear();
    systemState_ = QStringLiteral("UNKNOWN");
    btTreeState_.clear();
    btCurrentNode_.clear();
    btMessage_.clear();
    heartbeat_ = 0;
    lastStatusAt_ = {};

    if (!statusSubscriber_) {
        return;
    }

    connect(statusSubscriber_, &ZMQStatusSubscriber::axisMotionUpdated,
            this, [this](QVector<double> positions,
                         QVector<double> velocities,
                         QVector<double> cmdPositions) {
                axisPositions_ = std::move(positions);
                axisVelocities_ = std::move(velocities);
                axisCommandPositions_ = std::move(cmdPositions);
                lastStatusAt_ = QDateTime::currentDateTimeUtc();
            }, Qt::QueuedConnection);
    connect(statusSubscriber_, &ZMQStatusSubscriber::axisServoStatesUpdated,
            this, [this](QVector<quint8> enabled) {
                servoStates_ = std::move(enabled);
                lastStatusAt_ = QDateTime::currentDateTimeUtc();
            }, Qt::QueuedConnection);
    connect(statusSubscriber_, &ZMQStatusSubscriber::heartbeatReceived,
            this, [this](quint64 heartbeat) {
                heartbeat_ = heartbeat;
                lastStatusAt_ = QDateTime::currentDateTimeUtc();
            }, Qt::QueuedConnection);
    connect(statusSubscriber_, &ZMQStatusSubscriber::taskSchedulingUpdated,
            this, [this](const QString &state) {
                systemState_ = state;
                lastStatusAt_ = QDateTime::currentDateTimeUtc();
            }, Qt::QueuedConnection);
    connect(statusSubscriber_, &ZMQStatusSubscriber::btStatusUpdated,
            this, [this](const QString &treeState,
                         const QString &currentNode,
                         const QString &message) {
                btTreeState_ = treeState;
                btCurrentNode_ = currentNode;
                btMessage_ = message;
                lastStatusAt_ = QDateTime::currentDateTimeUtc();
            }, Qt::QueuedConnection);
}

void MujocoFrameServer::handleConnection()
{
    while (server_.hasPendingConnections()) {
        QTcpSocket *socket = server_.nextPendingConnection();
        if (!socket) {
            continue;
        }

        connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
            if (socket->property("requestHandled").toBool()) {
                return;
            }
            if (socket->bytesAvailable() > kMaxRequestBytes) {
                socket->setProperty("requestHandled", true);
                sendJson(socket,
                         QByteArrayLiteral("413 Payload Too Large"),
                         jsonError(QStringLiteral("request is too large")));
                socket->disconnectFromHost();
                return;
            }

            const QByteArray request = socket->peek(kMaxRequestBytes);
            const int headerEnd = request.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const int contentLength = requestContentLength(request, headerEnd);
            if (contentLength < 0 || contentLength > kMaxRequestBytes - headerEnd - 4) {
                socket->setProperty("requestHandled", true);
                sendJson(socket,
                         QByteArrayLiteral("400 Bad Request"),
                         jsonError(QStringLiteral("invalid request body length")));
                socket->disconnectFromHost();
                return;
            }
            if (request.size() < headerEnd + 4 + contentLength) {
                return;
            }
            socket->setProperty("requestHandled", true);
            handleRequest(socket);
        });

        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void MujocoFrameServer::handleRequest(QTcpSocket *socket)
{
    const QByteArray request = socket->read(kMaxRequestBytes);
    const int headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QByteArray requestLine = request.left(request.indexOf('\n')).trimmed();
    const QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 2) {
        sendJson(socket,
                 QByteArrayLiteral("400 Bad Request"),
                 jsonError(QStringLiteral("invalid HTTP request line")));
        socket->disconnectFromHost();
        return;
    }

    const QByteArray rawTarget = parts[1];
    const int queryIndex = rawTarget.indexOf('?');
    const QByteArray path = rawTarget.left(queryIndex >= 0 ? queryIndex : rawTarget.size());
    const int contentLength = requestContentLength(request, headerEnd);
    const QByteArray body = contentLength > 0 ? request.mid(headerEnd + 4, contentLength) : QByteArray();

    if (path == QByteArrayLiteral("/robot/motion")) {
        if (parts[0] != QByteArrayLiteral("POST")) {
            sendJson(socket,
                     QByteArrayLiteral("405 Method Not Allowed"),
                     jsonError(QStringLiteral("robot motion requires POST")));
            socket->disconnectFromHost();
            return;
        }
        handleMotionRequest(socket, body);
        return;
    }

    if (path == QByteArrayLiteral("/robot/status")) {
        if (parts[0] != QByteArrayLiteral("GET")) {
            sendJson(socket,
                     QByteArrayLiteral("405 Method Not Allowed"),
                     jsonError(QStringLiteral("robot status requires GET")));
            socket->disconnectFromHost();
            return;
        }
        handleRobotStatusRequest(socket);
        return;
    }

    if (parts[0] != QByteArrayLiteral("GET")) {
        sendJson(socket,
                 QByteArrayLiteral("405 Method Not Allowed"),
                 jsonError(QStringLiteral("only GET is supported for this endpoint")));
        socket->disconnectFromHost();
        return;
    }

    if (path == QByteArrayLiteral("/mujoco/latest.jpg")) {
        if (!view_) {
            sendJson(socket,
                     QByteArrayLiteral("503 Service Unavailable"),
                     jsonError(QStringLiteral("MuJoCo view is unavailable")));
            socket->disconnectFromHost();
            return;
        }

        const QImage image = view_->captureFrame();
        if (image.isNull()) {
            sendJson(socket,
                     QByteArrayLiteral("503 Service Unavailable"),
                     jsonError(QStringLiteral("MuJoCo framebuffer is unavailable")));
            socket->disconnectFromHost();
            return;
        }

        QByteArray jpeg;
        QBuffer buffer(&jpeg);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "JPG", 85)) {
            sendJson(socket,
                     QByteArrayLiteral("500 Internal Server Error"),
                     jsonError(QStringLiteral("failed to encode JPEG")));
            socket->disconnectFromHost();
            return;
        }

        sendResponse(socket,
                     QByteArrayLiteral("200 OK"),
                     QByteArrayLiteral("image/jpeg"),
                     jpeg);
        socket->disconnectFromHost();
        return;
    }

    if (path == QByteArrayLiteral("/mujoco/status")) {
        QJsonObject status;
        status.insert(QStringLiteral("ok"), true);
        status.insert(QStringLiteral("modelLoaded"), view_ && view_->isModelLoaded());
        status.insert(QStringLiteral("width"), view_ ? view_->width() : 0);
        status.insert(QStringLiteral("height"), view_ ? view_->height() : 0);
        status.insert(QStringLiteral("capturedAt"),
                      QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        sendJson(socket,
                 QByteArrayLiteral("200 OK"),
                 QJsonDocument(status).toJson(QJsonDocument::Compact));
        socket->disconnectFromHost();
        return;
    }

    if (path == QByteArrayLiteral("/") || path == QByteArrayLiteral("/mujoco")) {
        QJsonObject help;
        help.insert(QStringLiteral("ok"), true);
        help.insert(QStringLiteral("image"), QStringLiteral("GET /mujoco/latest.jpg"));
        help.insert(QStringLiteral("status"), QStringLiteral("GET /mujoco/status"));
        help.insert(QStringLiteral("robotStatus"), QStringLiteral("GET /robot/status"));
        help.insert(QStringLiteral("robotMotion"), QStringLiteral("POST /robot/motion"));
        sendJson(socket,
                 QByteArrayLiteral("200 OK"),
                 QJsonDocument(help).toJson(QJsonDocument::Compact));
        socket->disconnectFromHost();
        return;
    }

    sendJson(socket,
             QByteArrayLiteral("404 Not Found"),
             jsonError(QStringLiteral("endpoint not found")));
    socket->disconnectFromHost();
}

void MujocoFrameServer::handleRobotStatusRequest(QTcpSocket *socket)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const bool statusFresh = !lastStatusAt_.isNull() &&
                             lastStatusAt_.msecsTo(now) <= 2000;
    QJsonObject status;
    status.insert(QStringLiteral("ok"), true);
    status.insert(QStringLiteral("available"), statusFresh);
    status.insert(QStringLiteral("heartbeat"), static_cast<qint64>(heartbeat_));
    status.insert(QStringLiteral("systemState"), systemState_);
    status.insert(QStringLiteral("btTreeState"), btTreeState_);
    status.insert(QStringLiteral("btCurrentNode"), btCurrentNode_);
    status.insert(QStringLiteral("btMessage"), btMessage_);
    status.insert(QStringLiteral("motionId"), static_cast<qint64>(motionSequence_));
    status.insert(QStringLiteral("lastStatusAt"),
                  lastStatusAt_.isNull()
                      ? QString()
                      : lastStatusAt_.toString(Qt::ISODateWithMs));
    status.insert(QStringLiteral("lastMotionAt"),
                  lastMotionAt_.isNull()
                      ? QString()
                      : lastMotionAt_.toString(Qt::ISODateWithMs));

    QJsonArray positions;
    QJsonArray velocities;
    QJsonArray commandPositions;
    bool moving = false;
    // MuJoCo feedback can contain small numerical settling/jitter velocities.
    // Treat only meaningful axis motion as active so the wait tool can settle.
    constexpr double kMotionEpsilon = 2.0e-2;
    for (double value : axisPositions_) {
        positions.append(value);
    }
    for (double value : axisVelocities_) {
        velocities.append(value);
        moving = moving || std::abs(value) > kMotionEpsilon;
    }
    for (double value : axisCommandPositions_) {
        commandPositions.append(value);
    }
    status.insert(QStringLiteral("positions"), positions);
    status.insert(QStringLiteral("velocities"), velocities);
    status.insert(QStringLiteral("commandPositions"), commandPositions);
    QJsonArray servos;
    for (quint8 enabled : servoStates_) {
        servos.append(enabled != 0);
    }
    status.insert(QStringLiteral("servoEnabled"), servos);
    status.insert(QStringLiteral("moving"), moving);

    sendJson(socket,
             QByteArrayLiteral("200 OK"),
             QJsonDocument(status).toJson(QJsonDocument::Compact));
    socket->disconnectFromHost();
}

void MujocoFrameServer::handleMotionRequest(QTcpSocket *socket, const QByteArray &body)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        sendJson(socket,
                 QByteArrayLiteral("400 Bad Request"),
                 jsonError(QStringLiteral("body must be a JSON object")));
        socket->disconnectFromHost();
        return;
    }

    const QJsonObject request = document.object();
    const QString command = request.value(QStringLiteral("command")).toString().trimmed();
    const QJsonValue argsValue = request.value(QStringLiteral("args"));
    if (command != QStringLiteral("MoveJ") && command != QStringLiteral("MoveAbsJ")) {
        sendJson(socket,
                 QByteArrayLiteral("400 Bad Request"),
                 jsonError(QStringLiteral("only MoveJ and MoveAbsJ are exposed")));
        socket->disconnectFromHost();
        return;
    }
    if (!argsValue.isArray()) {
        sendJson(socket,
                 QByteArrayLiteral("400 Bad Request"),
                 jsonError(QStringLiteral("args must be an array of numbers")));
        socket->disconnectFromHost();
        return;
    }

    const QJsonArray jsonArgs = argsValue.toArray();
    if (jsonArgs.isEmpty() || jsonArgs.size() > 16) {
        sendJson(socket,
                 QByteArrayLiteral("400 Bad Request"),
                 jsonError(QStringLiteral("args count is outside the supported range")));
        socket->disconnectFromHost();
        return;
    }

    QVector<double> args;
    args.reserve(jsonArgs.size());
    for (const QJsonValue &value : jsonArgs) {
        if (!value.isDouble() || !std::isfinite(value.toDouble())) {
            sendJson(socket,
                     QByteArrayLiteral("400 Bad Request"),
                     jsonError(QStringLiteral("all args must be finite numbers")));
            socket->disconnectFromHost();
            return;
        }
        args.append(value.toDouble());
    }

    if (command == QStringLiteral("MoveJ")) {
        if (args.size() != 7 || args[6] <= 0.0 || args[6] > 1.0) {
            sendJson(socket,
                     QByteArrayLiteral("400 Bad Request"),
                     jsonError(QStringLiteral("MoveJ requires [x,y,z,rx,ry,rz,velocity] with 0 < velocity <= 1")));
            socket->disconnectFromHost();
            return;
        }
    } else {
        const int count = static_cast<int>(std::llround(args[0]));
        if (count <= 0 || count > 15 || args.size() != count + 1) {
            sendJson(socket,
                     QByteArrayLiteral("400 Bad Request"),
                     jsonError(QStringLiteral("MoveAbsJ requires [count,j1,...,jN]")));
            socket->disconnectFromHost();
            return;
        }
    }

    QString reply;
    QString error;
    const bool transportOk = sendMotionCommand(command, args, &reply, &error);
    const bool accepted = transportOk && !reply.startsWith(QStringLiteral("ERROR"));
    const quint64 motionId = accepted ? ++motionSequence_ : motionSequence_;
    if (accepted) {
        lastMotionAt_ = QDateTime::currentDateTimeUtc();
    }
    QJsonObject response;
    response.insert(QStringLiteral("ok"), accepted);
    response.insert(QStringLiteral("command"), command);
    response.insert(QStringLiteral("reply"), reply);
    response.insert(QStringLiteral("motionId"), static_cast<qint64>(motionId));
    QJsonArray normalizedArgs;
    for (const double arg : args) {
        normalizedArgs.append(arg);
    }
    response.insert(QStringLiteral("args"), normalizedArgs);
    if (!transportOk) {
        response.insert(QStringLiteral("error"), error);
        sendJson(socket,
                 QByteArrayLiteral("503 Service Unavailable"),
                 QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        sendJson(socket,
                 QByteArrayLiteral("200 OK"),
                 QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    socket->disconnectFromHost();
}

bool MujocoFrameServer::sendMotionCommand(const QString &command,
                                          const QVector<double> &args,
                                          QString *reply,
                                          QString *error)
{
    try {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::req);
        socket.set(zmq::sockopt::sndtimeo, 5000);
        socket.set(zmq::sockopt::rcvtimeo, 5000);
        socket.set(zmq::sockopt::linger, 0);
        const std::string endpoint =
            "tcp://" + motionHost_.toStdString() + ":" + std::to_string(motionPort_);
        socket.connect(endpoint);

        zrcs_message::MotionCommand motion;
        motion.set_command(command.toStdString());
        for (const double arg : args) {
            motion.add_args(arg);
        }
        std::string serialized;
        if (!motion.SerializeToString(&serialized)) {
            if (error) {
                *error = QStringLiteral("failed to serialize MotionCommand");
            }
            return false;
        }

        zmq::message_t request(serialized.data(), serialized.size());
        if (!socket.send(request, zmq::send_flags::none)) {
            if (error) {
                *error = QStringLiteral("failed to send MotionCommand");
            }
            return false;
        }

        zmq::message_t response;
        if (!socket.recv(response, zmq::recv_flags::none)) {
            if (error) {
                *error = QStringLiteral("no response from zrcsnrt within 5000 ms");
            }
            return false;
        }
        if (reply) {
            *reply = QString::fromUtf8(static_cast<const char *>(response.data()),
                                       static_cast<int>(response.size()));
        }
        return true;
    } catch (const zmq::error_t &exception) {
        if (error) {
            *error = QString::fromUtf8(exception.what());
        }
        return false;
    }
}

void MujocoFrameServer::sendResponse(QTcpSocket *socket,
                                     const QByteArray &status,
                                     const QByteArray &contentType,
                                     const QByteArray &body)
{
    if (!socket) {
        return;
    }

    QByteArray response;
    response.reserve(body.size() + 256);
    response += "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    socket->write(response);
}

void MujocoFrameServer::sendJson(QTcpSocket *socket,
                                 const QByteArray &status,
                                 const QByteArray &json)
{
    sendResponse(socket, status, QByteArrayLiteral("application/json"), json);
}
