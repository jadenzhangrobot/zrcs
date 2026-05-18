#include "MotionMainWindow.h"
#include <QSettings>
#include <QHBoxLayout>
#include <QDebug>
#include <cmath>

MotionMainWindow::MotionMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    connectSignals();

    const QString host = (editServerIp_ && !editServerIp_->text().trimmed().isEmpty())
        ? editServerIp_->text().trimmed()
        : QStringLiteral("localhost");
    createTransport(host);
}

MotionMainWindow::~MotionMainWindow()
{
    destroyTransport();
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

    // Per-axis set-origin buttons
    btnOriginX_ = findChild<QPushButton*>("btn_origin_x");
    btnOriginY_ = findChild<QPushButton*>("btn_origin_y");
    btnOriginZ_ = findChild<QPushButton*>("btn_origin_z");
    btnOriginA_ = findChild<QPushButton*>("btn_origin_a");
    btnOriginB_ = findChild<QPushButton*>("btn_origin_b");

    // Per-axis position labels
    lblPosX_ = findChild<QLabel*>("lbl_pos_x");
    lblPosY_ = findChild<QLabel*>("lbl_pos_y");
    lblPosZ_ = findChild<QLabel*>("lbl_pos_z");
    lblPosA_ = findChild<QLabel*>("lbl_pos_a");
    lblPosB_ = findChild<QLabel*>("lbl_pos_b");

    // XY jog
    btnJogNegX_ = findChild<QPushButton*>("pushButton_4");
    btnJogPosX_ = findChild<QPushButton*>("pushButton");
    btnJogNegY_ = findChild<QPushButton*>("pushButton_2");
    btnJogPosY_ = findChild<QPushButton*>("pushButton_3");

    // Z jog
    btnJogPosZ_ = findChild<QPushButton*>("pushButton_5");
    btnJogNegZ_ = findChild<QPushButton*>("pushButton_11");

    // alpha/beta jog
    btnJogNegA_ = findChild<QPushButton*>("pushButton_15");
    btnJogPosA_ = findChild<QPushButton*>("pushButton_13");
    btnJogNegB_ = findChild<QPushButton*>("pushButton_14");
    btnJogPosB_ = findChild<QPushButton*>("pushButton_16");

    // Jog mode and controls
    jogModeCombo_     = findChild<QComboBox*>("comboBox_jogMode");
    overrideSlider_   = findChild<QSlider*>("slider_overrideRatio");
    overrideLabel_    = findChild<QLabel*>("label_overrideValue");
    taskSchedStateLabel_ = findChild<QLabel*>("label_taskSchedState");
    btnTaskRun_       = findChild<QPushButton*>("pushButton_taskRun");
    btnTaskStop_      = findChild<QPushButton*>("pushButton_taskStop");
    btnTaskReset_     = findChild<QPushButton*>("pushButton_taskReset");
    stepDistXySlider_ = findChild<QSlider*>("slider_dist_xy");
    stepDistXyLabel_  = findChild<QLabel*>("label_dist_xy_val");
    stepDistZSlider_  = findChild<QSlider*>("slider_dist_z");
    stepDistZLabel_   = findChild<QLabel*>("label_dist_z_val");
    angleAbSlider_    = findChild<QSlider*>("slider_angle_ab");
    angleAbLabel_     = findChild<QLabel*>("label_angle_ab_val");

    // Homing
    btnHomeX_   = findChild<QPushButton*>("pushButton_17");
    btnHomeY_   = findChild<QPushButton*>("pushButton_18");
    btnHomeZ_   = findChild<QPushButton*>("pushButton_19");
    btnHomeA_   = findChild<QPushButton*>("pushButton_20");
    btnHomeB_   = findChild<QPushButton*>("pushButton_21");
    btnHomeAll_ = findChild<QPushButton*>("pushButton_22");

    // Settings
    editServerIp_ = findChild<QLineEdit*>("lineEdit_serverIp");
    editXRatio_ = findChild<QLineEdit*>("lineEdit");
    editYRatio_ = findChild<QLineEdit*>("lineEdit_2");
    editZRatio_ = findChild<QLineEdit*>("lineEdit_3");
    editARatio_ = findChild<QLineEdit*>("lineEdit_4");
    editBRatio_ = findChild<QLineEdit*>("lineEdit_5");
    btnConnect_ = findChild<QPushButton*>("pushButton_connect");
    btnSave_      = findChild<QPushButton*>("pushButton_6");

    // Status bar widgets
    positionLabel_ = new QLabel("X: --  Y: --  Z: --  a: --  b: --");
    zmqStatusLabel_ = new QLabel("ZMQ: Not Connected");
    statusBar()->addWidget(zmqStatusLabel_);
    statusBar()->addPermanentWidget(positionLabel_);

    // Initialize: send default override ratio
    if (overrideSlider_ && overrideSlider_->value() == 100) {
        setOverrideRatio(100);
    }

    updateTaskSchedulingDisplay(QStringLiteral("--"), false);

    // Load saved settings
    QSettings settings("ZRCS", "MotionGui");
    if (editServerIp_) editServerIp_->setText(settings.value("network/host", "localhost").toString());
    if (editXRatio_) editXRatio_->setText(settings.value("encoder/x", "").toString());
    if (editYRatio_) editYRatio_->setText(settings.value("encoder/y", "").toString());
    if (editZRatio_) editZRatio_->setText(settings.value("encoder/z", "").toString());
    if (editARatio_) editARatio_->setText(settings.value("encoder/alpha", "").toString());
    if (editBRatio_) editBRatio_->setText(settings.value("encoder/beta", "").toString());
}

void MotionMainWindow::connectSignals()
{
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

    // Jog buttons: mode-aware (连动/点动)
    auto wireJogButton = [this](QPushButton* btn, int axisId, bool positive) {
        if (!btn) return;
        connect(btn, &QPushButton::pressed, this, [this, axisId, positive]() {
            if (isStepJogMode()) {
                stepJog(axisId, positive);
            } else {
                jogStart(axisId, positive);
            }
        });
        connect(btn, &QPushButton::released, this, [this]() {
            if (!isStepJogMode()) {
                jogStop();
            }
        });
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

    // Jog mode ComboBox
    if (jogModeCombo_) {
        connect(jogModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
            jogStop();  // safety: stop continuous motion when switching modes
            bool stepMode = (index == 1);
            if (stepDistXySlider_) stepDistXySlider_->setEnabled(stepMode);
            if (stepDistXyLabel_)  stepDistXyLabel_->setEnabled(stepMode);
            if (stepDistZSlider_)  stepDistZSlider_->setEnabled(stepMode);
            if (stepDistZLabel_)   stepDistZLabel_->setEnabled(stepMode);
            if (angleAbSlider_)    angleAbSlider_->setEnabled(stepMode);
            if (angleAbLabel_)     angleAbLabel_->setEnabled(stepMode);
        });
    }

    // Override ratio slider
    if (overrideSlider_) {
        connect(overrideSlider_, &QSlider::valueChanged, this, [this](int value) {
            if (overrideLabel_) overrideLabel_->setText(QString::number(value) + "%");
            setOverrideRatio(value);
        });
    }

    if (btnTaskRun_) {
        connect(btnTaskRun_, &QPushButton::clicked, this, [this]() {
            sendCommand("SYS_RUN", {});
        });
    }
    if (btnTaskStop_) {
        connect(btnTaskStop_, &QPushButton::clicked, this, [this]() {
            sendCommand("SYS_STOP", {});
        });
    }
    if (btnTaskReset_) {
        connect(btnTaskReset_, &QPushButton::clicked, this, [this]() {
            sendCommand("SYS_RESET", {});
        });
    }

    // Step distance sliders
    if (stepDistXySlider_) {
        connect(stepDistXySlider_, &QSlider::valueChanged, this, [this](int value) {
            if (stepDistXyLabel_) stepDistXyLabel_->setText(stepDistanceText(value));
        });
    }
    if (stepDistZSlider_) {
        connect(stepDistZSlider_, &QSlider::valueChanged, this, [this](int value) {
            if (stepDistZLabel_) stepDistZLabel_->setText(stepDistanceText(value));
        });
    }
    if (angleAbSlider_) {
        connect(angleAbSlider_, &QSlider::valueChanged, this, [this](int value) {
            if (angleAbLabel_) angleAbLabel_->setText(angleStepText(value));
        });
    }

    // Homing
    if (btnHomeX_)   connect(btnHomeX_,   &QPushButton::clicked, this, [this]() { homeAxis(0); });
    if (btnHomeY_)   connect(btnHomeY_,   &QPushButton::clicked, this, [this]() { homeAxis(1); });
    if (btnHomeZ_)   connect(btnHomeZ_,   &QPushButton::clicked, this, [this]() { homeAxis(2); });
    if (btnHomeA_)   connect(btnHomeA_,   &QPushButton::clicked, this, [this]() { homeAxis(3); });
    if (btnHomeB_)   connect(btnHomeB_,   &QPushButton::clicked, this, [this]() { homeAxis(4); });
    if (btnHomeAll_) connect(btnHomeAll_, &QPushButton::clicked, this, &MotionMainWindow::homeAllAxes);

    // Settings save
    if (btnConnect_)   connect(btnConnect_,   &QPushButton::clicked, this, &MotionMainWindow::connectToConfiguredHost);
    if (btnSave_)      connect(btnSave_,      &QPushButton::clicked, this, &MotionMainWindow::saveSettings);

    // Per-axis set-origin buttons
    if (btnOriginX_) connect(btnOriginX_, &QPushButton::clicked, this, [this]() { setAxisAsOrigin(0); });
    if (btnOriginY_) connect(btnOriginY_, &QPushButton::clicked, this, [this]() { setAxisAsOrigin(1); });
    if (btnOriginZ_) connect(btnOriginZ_, &QPushButton::clicked, this, [this]() { setAxisAsOrigin(2); });
    if (btnOriginA_) connect(btnOriginA_, &QPushButton::clicked, this, [this]() { setAxisAsOrigin(3); });
    if (btnOriginB_) connect(btnOriginB_, &QPushButton::clicked, this, [this]() { setAxisAsOrigin(4); });
}

void MotionMainWindow::createTransport(const QString& host)
{
    const QString normalizedHost = host.trimmed().isEmpty() ? QStringLiteral("localhost") : host.trimmed();

    destroyTransport();

    zmqClient_ = new MotionZmqClient(normalizedHost, 5555, 5000, this);
    statusSub_ = new MotionStatusSubscriber(normalizedHost, 5556, this);

    connect(zmqClient_, &MotionZmqClient::connected, this, &MotionMainWindow::onZmqConnected);
    connect(zmqClient_, &MotionZmqClient::disconnected, this, &MotionMainWindow::onZmqDisconnected);
    connect(zmqClient_, &MotionZmqClient::errorOccurred, this, &MotionMainWindow::onZmqError);
    connect(statusSub_, &MotionStatusSubscriber::statusUpdated, this, &MotionMainWindow::onStatusUpdated);
    connect(statusSub_, &MotionStatusSubscriber::taskSchedulingUpdated,
            this, &MotionMainWindow::onTaskSchedulingUpdated);

    if (zmqStatusLabel_) {
        zmqStatusLabel_->setText(QString("ZMQ: Connecting to %1").arg(normalizedHost));
    }
    updateTaskSchedulingDisplay(QStringLiteral("连接中"), false);

    zmqClient_->connectToServer();
    statusSub_->start();
}

void MotionMainWindow::destroyTransport()
{
    if (statusSub_) {
        statusSub_->stop();
        delete statusSub_;
        statusSub_ = nullptr;
    }

    if (zmqClient_) {
        zmqClient_->disconnectFromServer();
        delete zmqClient_;
        zmqClient_ = nullptr;
    }
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

void MotionMainWindow::setCurrentAsOrigin()
{
    sendCommand("SYS_SET_ORIGIN", {});
}

void MotionMainWindow::setAxisAsOrigin(int axisId)
{
    sendCommand("SYS_SET_AXIS_ORIGIN", {static_cast<double>(axisId)});
}

// ============================================================================
// Jog mode helpers
// ============================================================================

bool MotionMainWindow::isStepJogMode() const
{
    return jogModeCombo_ && jogModeCombo_->currentIndex() == 1;
}

double MotionMainWindow::stepDistance(int axisId) const
{
    QSlider* s = (axisId <= 1) ? stepDistXySlider_
               : (axisId == 2) ? stepDistZSlider_
               :                  angleAbSlider_;
    if (!s) return 0.0;
    int v = s->value();
    // Rotation axes (3,4): 10^(v-2) degrees (0.01 to 10 deg)
    // Linear axes:         10^(v-6) mm  (1nm=0.000001mm .. 10mm), RT user unit = mm
    return (axisId >= 3) ? std::pow(10.0, v - 2) : std::pow(10.0, v - 6);
}

QString MotionMainWindow::stepDistanceText(int value) const
{
    // value 0-7 maps to 1nm .. 10mm
    if (value <= 2) {
        return QString("%1 nm").arg(static_cast<int>(std::pow(10, value)));
    } else if (value <= 5) {
        return QString("%1 um").arg(static_cast<int>(std::pow(10, value - 3)));
    } else {
        return QString("%1 mm").arg(static_cast<int>(std::pow(10, value - 6)));
    }
}

QString MotionMainWindow::angleStepText(int value) const
{
    // value 0-3 maps to 0.01, 0.1, 1, 10 degrees
    const QStringList labels = {"0.01 deg", "0.1 deg", "1 deg", "10 deg"};
    if (value >= 0 && value < labels.size()) return labels.at(value);
    return QString::number(static_cast<int>(std::pow(10, value - 2))) + " deg";
}

void MotionMainWindow::stepJog(int axisId, bool positive)
{
    double dist = stepDistance(axisId);
    if (dist <= 0.0) return;
    double distance = positive ? dist : -dist;
    sendCommand("JogJ", {static_cast<double>(axisId), distance});
}

void MotionMainWindow::setOverrideRatio(int percent)
{
    sendCommand("SYS_SET_MULTIPLIER", {static_cast<double>(percent)});
}

// ============================================================================
// ZMQ status slots
// ============================================================================

void MotionMainWindow::onZmqConnected()
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Connected");
    updateTaskSchedulingDisplay(QStringLiteral("等待状态"), true);
}

void MotionMainWindow::onZmqDisconnected()
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Disconnected");
    updateTaskSchedulingDisplay(QStringLiteral("未连接"), false);
}

void MotionMainWindow::onZmqError(const QString& error)
{
    if (zmqStatusLabel_) zmqStatusLabel_->setText("ZMQ: Error");
    updateTaskSchedulingDisplay(QStringLiteral("错误"), false);
    statusBar()->showMessage(error, 3000);
}

void MotionMainWindow::onStatusUpdated(const QVector<AxisStatusData>& axes, quint64 heartbeat)
{
    Q_UNUSED(heartbeat);

    // Status bar summary
    if (positionLabel_) {
        QStringList parts;
        const QString names[] = {"X", "Y", "Z", "a", "b"};
        for (int i = 0; i < axes.size() && i < 5; ++i)
            parts << QString("%1: %2").arg(names[i]).arg(axes[i].position, 0, 'f', 3);
        positionLabel_->setText(parts.join("  "));
    }

    // Per-axis labels in panels
    QLabel* const panelLabels[] = {lblPosX_, lblPosY_, lblPosZ_, lblPosA_, lblPosB_};
    const QString units[]       = {" mm",   " mm",   " mm",   " deg",  " deg"};
    const QString prefixes[]    = {"X: ",   "Y: ",   "Z: ",   "α: ",   "β: "};
    for (int i = 0; i < axes.size() && i < 5; ++i) {
        if (panelLabels[i])
            panelLabels[i]->setText(
                prefixes[i] + QString::number(axes[i].position, 'f', 3) + units[i]);
    }
}

void MotionMainWindow::onTaskSchedulingUpdated(const QString& state)
{
    updateTaskSchedulingDisplay(state, true);
}

void MotionMainWindow::updateTaskSchedulingDisplay(const QString& state, bool connected)
{
    if (taskSchedStateLabel_) {
        taskSchedStateLabel_->setText(state);
    }

    if (btnTaskRun_) {
        btnTaskRun_->setEnabled(connected && state != "RUN");
    }
    if (btnTaskStop_) {
        btnTaskStop_->setEnabled(connected && state != "STOP");
    }
    if (btnTaskReset_) {
        btnTaskReset_->setEnabled(connected && state != "RESET");
    }
}

// ============================================================================
// Settings
// ============================================================================

void MotionMainWindow::saveSettings()
{
    QSettings settings("ZRCS", "MotionGui");
    const QString host = (editServerIp_ && !editServerIp_->text().trimmed().isEmpty())
        ? editServerIp_->text().trimmed()
        : QStringLiteral("localhost");

    if (editServerIp_) {
        editServerIp_->setText(host);
        settings.setValue("network/host", host);
    }
    if (editXRatio_) settings.setValue("encoder/x", editXRatio_->text());
    if (editYRatio_) settings.setValue("encoder/y", editYRatio_->text());
    if (editZRatio_) settings.setValue("encoder/z", editZRatio_->text());
    if (editARatio_) settings.setValue("encoder/alpha", editARatio_->text());
    if (editBRatio_) settings.setValue("encoder/beta", editBRatio_->text());

    createTransport(host);

    statusBar()->showMessage(QString("Settings saved, reconnecting to %1").arg(host), 3000);
}

void MotionMainWindow::connectToConfiguredHost()
{
    const QString host = (editServerIp_ && !editServerIp_->text().trimmed().isEmpty())
        ? editServerIp_->text().trimmed()
        : QStringLiteral("localhost");

    if (editServerIp_) {
        editServerIp_->setText(host);
    }

    QSettings settings("ZRCS", "MotionGui");
    settings.setValue("network/host", host);

    createTransport(host);
    statusBar()->showMessage(QString("Connecting to %1").arg(host), 3000);
}
