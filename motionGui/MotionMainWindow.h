#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QVector>
#include "ui_MainWindow.h"
#include "MotionZmqClient.h"
#include "MotionStatusSubscriber.h"

class MotionMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MotionMainWindow(QWidget* parent = nullptr);
    ~MotionMainWindow();

private slots:
    void onZmqConnected();
    void onZmqDisconnected();
    void onZmqError(const QString& error);
    void onStatusUpdated(const QVector<AxisStatusData>& axes, quint64 heartbeat);
    void saveSettings();

private:
    void setupUi();
    void connectSignals();
    void sendCommand(const QString& cmd, const QVector<double>& args = {});

    void enableAxis(int axisId);
    void disableAxis(int axisId);
    void resetAxis(int axisId);
    void errorClearAxis(int axisId);
    void jogStart(int axisId, bool positive);
    void jogStop();
    void homeAxis(int axisId);
    void homeAllAxes();


    MotionZmqClient* zmqClient_ = nullptr;
    MotionStatusSubscriber* statusSub_ = nullptr;

    // Control tab - X axis
    QPushButton* btnXEnable_ = nullptr;
    QPushButton* btnXDisable_ = nullptr;
    QPushButton* btnXErrorClear_ = nullptr;
    QPushButton* btnXReset_ = nullptr;
    // Control tab - Y axis
    QPushButton* btnYEnable_ = nullptr;
    QPushButton* btnYDisable_ = nullptr;
    QPushButton* btnYErrorClear_ = nullptr;
    QPushButton* btnYReset_ = nullptr;
    // Control tab - Z axis
    QPushButton* btnZEnable_ = nullptr;
    QPushButton* btnZDisable_ = nullptr;
    QPushButton* btnZErrorClear_ = nullptr;
    QPushButton* btnZReset_ = nullptr;
    // Control tab - alpha axis
    QPushButton* btnAEnable_ = nullptr;
    QPushButton* btnADisable_ = nullptr;
    QPushButton* btnAErrorClear_ = nullptr;
    QPushButton* btnAReset_ = nullptr;
    // Control tab - beta axis
    QPushButton* btnBEnable_ = nullptr;
    QPushButton* btnBDisable_ = nullptr;
    QPushButton* btnBErrorClear_ = nullptr;
    QPushButton* btnBReset_ = nullptr;

    // Control tab - XY jog
    QPushButton* btnJogNegX_ = nullptr;
    QPushButton* btnJogPosX_ = nullptr;
    QPushButton* btnJogNegY_ = nullptr;
    QPushButton* btnJogPosY_ = nullptr;
    QSlider* xyStepSlider_ = nullptr;

    // Control tab - Z jog
    QPushButton* btnJogPosZ_ = nullptr;
    QPushButton* btnJogNegZ_ = nullptr;
    QSlider* zStepSlider_ = nullptr;

    // Control tab - alpha/beta jog
    QPushButton* btnJogNegA_ = nullptr;
    QPushButton* btnJogPosA_ = nullptr;
    QPushButton* btnJogNegB_ = nullptr;
    QPushButton* btnJogPosB_ = nullptr;
    QSlider* angleStepSlider_ = nullptr;

    // Homing tab
    QPushButton* btnHomeX_ = nullptr;
    QPushButton* btnHomeY_ = nullptr;
    QPushButton* btnHomeZ_ = nullptr;
    QPushButton* btnHomeA_ = nullptr;
    QPushButton* btnHomeB_ = nullptr;
    QPushButton* btnHomeAll_ = nullptr;

    // Settings tab
    QLineEdit* editXRatio_ = nullptr;
    QLineEdit* editYRatio_ = nullptr;
    QLineEdit* editZRatio_ = nullptr;
    QLineEdit* editARatio_ = nullptr;
    QLineEdit* editBRatio_ = nullptr;
    QPushButton* btnSave_ = nullptr;

    // Status bar
    QLabel* positionLabel_ = nullptr;
    QLabel* zmqStatusLabel_ = nullptr;
};
