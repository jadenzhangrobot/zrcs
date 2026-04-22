#include "MotionMainWindow.h"
#include <QSettings>
#include <QHBoxLayout>
#include <QDebug>

MotionMainWindow::MotionMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();

    zmqClient_ = new MotionZmqClient("localhost", 5555, 5000, this);
    statusSub_ = new MotionStatusSubscriber("localhost", 5556, this);

    connectSignals();

    zmqClient_->connectToServer();
    statusSub_->start();
}

MotionMainWindow::~MotionMainWindow()
{
    if (statusSub_) statusSub_->stop();
    if (zmqClient_) zmqClient_->disconnectFromServer();
}

void MotionMainWindow::setupUi()
{
    Ui::MainWindow ui;
    ui.setupUi(this);
    setWindowTitle("ZRCS Motion Control Panel");

    // X axis
    btnXEnable_     = findChild<QPushButton*>("pushButton_enable");
    btnXDisable_    = findChild<QPushButton*>("pushButton_disable");
    btnXErrorClear_ = findChild<QPushButton*>("pushButton_errorClear");
    btnXReset_      = findChild<QPushButton*>("pushButton_reset");

    // Y axis
    btnYEnable_     = findChild<QPushButton*>("pushButton_7");
    btnYDisable_    = findChild<QPushButton*>("pushButton_8");
    btnYErrorClear_ = findChild<QPushButton*>("pushButton_9");
    btnYReset_      = findChild<QPushButton*>("pushButton_10");

    // Z axis
    btnZEnable_     = findChild<QPushButton*>("pushButton_26");
    btnZDisable_    = findChild<QPushButton*>("pushButton_27");
    btnZErrorClear_ = findChild<QPushButton*>("pushButton_28");
    btnZReset_      = findChild<QPushButton*>("pushButton_29");

    // alpha axis
    btnAEnable_     = findChild<QPushButton*>("pushButton_34");
    btnADisable_    = findChild<QPushButton*>("pushButton_35");
    btnAErrorClear_ = findChild<QPushButton*>("pushButton_36");
    btnAReset_      = findChild<QPushButton*>("pushButton_37");

    // beta axis
    btnBEnable_     = findChild<QPushButton*>("pushButton_30");
    btnBDisable_    = findChild<QPushButton*>("pushButton_31");
    btnBErrorClear_ = findChild<QPushButton*>("pushButton_32");
    btnBReset_      = findChild<QPushButton*>("pushButton_33");

    // XY jog
    btnJogNegX_ = findChild<QPushButton*>("pushButton_4");
    btnJogPosX_ = findChild<QPushButton*>("pushButton");
    btnJogNegY_ = findChild<QPushButton*>("pushButton_2");
    btnJogPosY_ = findChild<QPushButton*>("pushButton_3");
    xyStepSlider_ = findChild<QSlider*>("horizontalSlider");

    // Z jog
    btnJogPosZ_ = findChild<QPushButton*>("pushButton_5");
    btnJogNegZ_ = findChild<QPushButton*>("pushButton_11");
    zStepSlider_ = findChild<QSlider*>("horizontalSlider_2");

    // alpha/beta jog
    btnJogNegA_ = findChild<QPushButton*>("pushButton_15");
    btnJogPosA_ = findChild<QPushButton*>("pushButton_13");
    btnJogNegB_ = findChild<QPushButton*>("pushButton_14");
    btnJogPosB_ = findChild<QPushButton*>("pushButton_16");
    angleStepSlider_ = findChild<QSlider*>("horizontalSlider_3");

    // Homing
    btnHomeX_   = findChild<QPushButton*>("pushButton_17");
    btnHomeY_   = findChild<QPushButton*>("pushButton_18");
    btnHomeZ_   = findChild<QPushButton*>("pushButton_19");
    btnHomeA_   = findChild<QPushButton*>("pushButton_20");
    btnHomeB_   = findChild<QPushButton*>("pushButton_21");
    btnHomeAll_ = findChild<QPushButton*>("pushButton_22");

    // Settings
    editXRatio_ = findChild<QLineEdit*>("lineEdit");
    editYRatio_ = findChild<QLineEdit*>("lineEdit_2");
    editZRatio_ = findChild<QLineEdit*>("lineEdit_3");
    editARatio_ = findChild<QLineEdit*>("lineEdit_4");
    editBRatio_ = findChild<QLineEdit*>("lineEdit_5");
    btnSave_    = findChild<QPushButton*>("pushButton_6");

    // Status bar widgets
    positionLabel_ = new QLabel("X: --  Y: --  Z: --  a: --  b: --");
    zmqStatusLabel_ = new QLabel("ZMQ: Not Connected");
    statusBar()->addWidget(zmqStatusLabel_);
    statusBar()->addPermanentWidget(positionLabel_);

    // Load saved settings
    QSettings settings("ZRCS", "MotionGui");
    if (editXRatio_) editXRatio_->setText(settings.value("encoder/x", "").toString());
    if (editYRatio_) editYRatio_->setText(settings.value("encoder/y", "").toString());
    if (editZRatio_) editZRatio_->setText(settings.value("encoder/z", "").toString());
    if (editARatio_) editARatio_->setText(settings.value("encoder/alpha", "").toString());
    if (editBRatio_) editBRatio_->setText(settings.value("encoder/beta", "").toString());
}

void MotionMainWindow::connectSignals()
{
    // ZMQ client
    connect(zmqClient_, &MotionZmqClient::connected, this, &MotionMainWindow::onZmqConnected);
    connect(zmqClient_, &MotionZmqClient::disconnected, this, &MotionMainWindow::onZmqDisconnected);
    connect(zmqClient_, &MotionZmqClient::errorOccurred, this, &MotionMainWindow::onZmqError);

    // Status subscriber
    connect(statusSub_, &MotionStatusSubscriber::statusUpdated, this, &MotionMainWindow::onStatusUpdated);

    // Axis enable/disable/reset buttons
    auto wireAxisButtons = [this](QPushButton* enable, QPushButton* disable,
                                   QPushButton* errorClear, QPushButton* reset, int axisId) {
        if (enable)     connect(enable, &QPushButton::clicked, this, [this, axisId]() { enableAxis(axisId); });
        if (disable)    connect(disable, &QPushButton::clicked, this, [this, axisId]() { disableAxis(axisId); });
        if (errorClear) connect(errorClear, &QPushButton::clicked, this, [this, axisId]() { errorClearAxis(axisId); });
        if (reset)      connect(reset, &QPushButton::clicked, this, [this, axisId]() { resetAxis(axisId); });
    };

    wireAxisButtons(btnXEnable_, btnXDisable_, btnXErrorClear_, btnXReset_, 0);
    wireAxisButtons(btnYEnable_, btnYDisable_, btnYErrorClear_, btnYReset_, 1);
    wireAxisButtons(btnZEnable_, btnZDisable_, btnZErrorClear_, btnZReset_, 2);
    wireAxisButtons(btnAEnable_, btnADisable_, btnAErrorClear_, btnAReset_, 3);
    wireAxisButtons(btnBEnable_, btnBDisable_, btnBErrorClear_, btnBReset_, 4);

    // XY jog (pressed = start, released = stop)
    auto wireJogButton = [this](QPushButton* btn, int axisId, bool positive) {
        if (!btn) return;
        connect(btn, &QPushButton::pressed, this, [this, axisId, positive]() { jogStart(axisId, positive); });
        connect(btn, &QPushButton::released, this, [this]() { jogStop(); });
    };

    wireJogButton(btnJogNegX_, 0, false);
    wireJogButton(btnJogPosX_, 0, true);
    wireJogButton(btnJogNegY_, 1, false);
    wireJogButton(btnJogPosY_, 1, true);

    // Z jog
    wireJogButton(btnJogPosZ_, 2, true);
    wireJogButton(btnJogNegZ_, 2, false);

    // alpha/beta jog
    wireJogButton(btnJogNegA_, 3, false);
    wireJogButton(btnJogPosA_, 3, true);
    wireJogButton(btnJogNegB_, 4, false);
    wireJogButton(btnJogPosB_, 4, true);

    // Homing
    if (btnHomeX_)   connect(btnHomeX_,   &QPushButton::clicked, this, [this]() { homeAxis(0); });
    if (btnHomeY_)   connect(btnHomeY_,   &QPushButton::clicked, this, [this]() { homeAxis(1); });
    if (btnHomeZ_)   connect(btnHomeZ_,   &QPushButton::clicked, this, [this]() { homeAxis(2); });
    if (btnHomeA_)   connect(btnHomeA_,   &QPushButton::clicked, this, [this]() { homeAxis(3); });
    if (btnHomeB_)   connect(btnHomeB_,   &QPushButton::clicked, this, [this]() { homeAxis(4); });
    if (btnHomeAll_) connect(btnHomeAll_, &QPushButton::clicked, this, &MotionMainWindow::homeAllAxes);

    // Settings save
    if (btnSave_) connect(btnSave_, &QPushButton::clicked, this, &MotionMainWindow::saveSettings);
}

// ============================================================================
// Command helpers
// ============================================================================

void MotionMainWindow::sendCommand(const QString& cmd, const QVector<double>& args)
{
    if (zmqClient_) {
        zmqClient_->sendCommand(cmd, args);
    }
}

void MotionMainWindow::enableAxis(int axisId)
{
    sendCommand("Enable", {static_cast<double>(axisId)});
}

void MotionMainWindow::disableAxis(int axisId)
{
    sendCommand("Disable", {static_cast<double>(axisId)});
}

void MotionMainWindow::resetAxis(int axisId)
{
    sendCommand("Reset", {static_cast<double>(axisId)});
}

void MotionMainWindow::errorClearAxis(int axisId)
{
    sendCommand("Reset", {static_cast<double>(axisId)});
}

void MotionMainWindow::jogStart(int axisId, bool positive)
{
    // args: [axisId, direction] — direction > 0 = positive, 0 = negative
    sendCommand("SYS_JOG_START", {static_cast<double>(axisId), positive ? 1.0 : 0.0});
}

void MotionMainWindow::jogStop()
{
    sendCommand("SYS_JOG_STOP", {});
}

void MotionMainWindow::homeAxis(int axisId)
{
    sendCommand("JogabsJ", {static_cast<double>(axisId), 0.0});
}

void MotionMainWindow::homeAllAxes()
{
    sendCommand("Movehome", {});
}

// ============================================================================
// ZMQ status slots
// ============================================================================

void MotionMainWindow::onZmqConnected()
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Connected");
}

void MotionMainWindow::onZmqDisconnected()
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Disconnected");
}

void MotionMainWindow::onZmqError(const QString& error)
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Error");
    statusBar()->showMessage(error, 3000);
}

void MotionMainWindow::onStatusUpdated(const QVector<AxisStatusData>& axes, quint64 heartbeat)
{
    Q_UNUSED(heartbeat);

    if (!positionLabel_) return;

    QStringList parts;
    const QString names[] = {"X", "Y", "Z", "a", "b"};
    for (int i = 0; i < axes.size() && i < 5; ++i) {
        parts << QString("%1: %2").arg(names[i]).arg(axes[i].position, 0, 'f', 3);
    }
    positionLabel_->setText(parts.join("  "));
}

// ============================================================================
// Settings
// ============================================================================

void MotionMainWindow::saveSettings()
{
    QSettings settings("ZRCS", "MotionGui");
    if (editXRatio_) settings.setValue("encoder/x", editXRatio_->text());
    if (editYRatio_) settings.setValue("encoder/y", editYRatio_->text());
    if (editZRatio_) settings.setValue("encoder/z", editZRatio_->text());
    if (editARatio_) settings.setValue("encoder/alpha", editARatio_->text());
    if (editBRatio_) settings.setValue("encoder/beta", editBRatio_->text());

    statusBar()->showMessage("Settings saved", 2000);
}
