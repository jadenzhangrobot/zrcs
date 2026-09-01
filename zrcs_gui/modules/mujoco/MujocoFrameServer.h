#pragma once

#include <QObject>
#include <QDateTime>
#include <QPointer>
#include <QString>
#include <QTcpServer>
#include <QPointer>
#include <QVector>

class MujocoVisualizer3D;
class QTcpSocket;
class ZMQStatusSubscriber;

/**
 * @brief Local, on-demand HTTP endpoint for MuJoCo screenshots.
 *
 * The server only binds to localhost. A frame is rendered when a client
 * requests /mujoco/latest.jpg; no background capture or disk cache is used.
 */
class MujocoFrameServer : public QObject {
    Q_OBJECT

public:
    explicit MujocoFrameServer(QObject *parent = nullptr);
    ~MujocoFrameServer() override;

    bool start(MujocoVisualizer3D *view,
               quint16 port = 8765,
               const QString &motionHost = QStringLiteral("127.0.0.1"),
               quint16 motionPort = 5555);
    void stop();
    bool isListening() const;
    quint16 port() const;
    void setMotionEndpoint(const QString &host, quint16 port);
    void setStatusSubscriber(ZMQStatusSubscriber *subscriber);

private:
    void handleConnection();
    void handleRequest(QTcpSocket *socket);
    void handleMotionRequest(QTcpSocket *socket, const QByteArray &body);
    void handleRobotStatusRequest(QTcpSocket *socket);
    bool sendMotionCommand(const QString &command,
                           const QVector<double> &args,
                           QString *reply,
                           QString *error);
    void sendResponse(QTcpSocket *socket,
                      const QByteArray &status,
                      const QByteArray &contentType,
                      const QByteArray &body);
    void sendJson(QTcpSocket *socket,
                  const QByteArray &status,
                  const QByteArray &json);

    QTcpServer server_;
    QPointer<MujocoVisualizer3D> view_;
    QPointer<ZMQStatusSubscriber> statusSubscriber_;
    QString motionHost_ = QStringLiteral("127.0.0.1");
    quint16 motionPort_ = 5555;
    QVector<double> axisPositions_;
    QVector<double> axisVelocities_;
    QVector<double> axisCommandPositions_;
    QVector<quint8> servoStates_;
    QString systemState_ = QStringLiteral("UNKNOWN");
    QString btTreeState_;
    QString btCurrentNode_;
    QString btMessage_;
    quint64 heartbeat_ = 0;
    quint64 motionSequence_ = 0;
    QDateTime lastStatusAt_;
    QDateTime lastMotionAt_;
};
