#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
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
    void setCurrentAsOrigin();
    void setAxisAsOrigin(int axisId);

    bool isStepJogMode() const;
    double stepDistance(int axisId) const;
    QString stepDistanceText(int value) const;
    QString angleStepText(int value) const;
    void stepJog(int axisId, bool positive);
    void setOverrideRatio(int percent);


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

    // Per-axis set-origin buttons
    QPushButton* btnOriginX_ = nullptr;
    QPushButton* btnOriginY_ = nullptr;
    QPushButton* btnOriginZ_ = nullptr;
    QPushButton* btnOriginA_ = nullptr;
    QPushButton* btnOriginB_ = nullptr;

    // Per-axis position labels
    QLabel* lblPosX_ = nullptr;
    QLabel* lblPosY_ = nullptr;
    QLabel* lblPosZ_ = nullptr;
    QLabel* lblPosA_ = nullptr;
    QLabel* lblPosB_ = nullptr;

    // Control tab - XY jog
    QPushButton* btnJogNegX_ = nullptr;
    QPushButton* btnJogPosX_ = nullptr;
    QPushButton* btnJogNegY_ = nullptr;
    QPushButton* btnJogPosY_ = nullptr;

    // Control tab - Z jog
    QPushButton* btnJogPosZ_ = nullptr;
    QPushButton* btnJogNegZ_ = nullptr;

    // Control tab - alpha/beta jog
    QPushButton* btnJogNegA_ = nullptr;
    QPushButton* btnJogPosA_ = nullptr;
    QPushButton* btnJogNegB_ = nullptr;
    QPushButton* btnJogPosB_ = nullptr;

    // Jog mode and controls
    QComboBox* jogModeCombo_     = nullptr;
    QSlider*   overrideSlider_   = nullptr;
    QLabel*    overrideLabel_    = nullptr;
    QSlider*   stepDistXySlider_ = nullptr;
    QLabel*    stepDistXyLabel_  = nullptr;
    QSlider*   stepDistZSlider_  = nullptr;
    QLabel*    stepDistZLabel_   = nullptr;
    QSlider*   angleAbSlider_    = nullptr;
    QLabel*    angleAbLabel_     = nullptr;

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
    QPushButton* btnSave_      = nullptr;

    // Status bar
    QLabel* positionLabel_ = nullptr;
    QLabel* zmqStatusLabel_ = nullptr;
};
