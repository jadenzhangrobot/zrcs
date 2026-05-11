#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QStatusBar>
#include <QTabWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <functional>
#include "shared_memory/NrtProcess.h"
#include "communication/ZmqClient.h"
#include "communication/ZmqStatusSubscriber.h"
#include "trajectory/TrajectoryVisualizer.h"
#include "command/CommandPanel.h"

class BehaviorTreePanel;

class StatusIndicator : public QWidget {
    Q_OBJECT
public:
    enum State { Idle, Running, Alarm, EStop };
    StatusIndicator(QWidget *parent = nullptr);
    void setState(State state);
    void setText(const QString &text);

private:
    void paintEvent(QPaintEvent *event) override;
    State currentState;
    QString statusText;
    QColor getColor() const;
};

class AxisPositionDisplay : public QWidget {
    Q_OBJECT
public:
    AxisPositionDisplay(const QString &axisName, QWidget *parent = nullptr);
    void updatePosition(double machine, double absolute, double relative);
    void updateDynamics(double velocity, double acceleration);
    void updateServoData(double torque, double followError, double temperature);

private:
    QLabel *machineLabel, *absoluteLabel, *relativeLabel;
    QLabel *velocityLabel, *accelerationLabel;
    QLabel *torqueLabel, *followErrorLabel, *tempLabel;
};

class JogControlPanel : public QWidget {
    Q_OBJECT
public:
    JogControlPanel(QWidget *parent = nullptr);
    void setAxisCount(int count);
    void setAxisPosition(int axis, double position);
    
signals:
    void jogPressed(int axis, int direction);
    void jogReleased(int axis);
    void stepSizeChanged(double size);
    void overrideChanged(int percent);
    void homeRequested(int axis);
    void homeAllRequested();
    void setCurrentAsOriginRequested(int axis);

private:
    void setupUI();
    void refreshAxisButtons();
    QVector<QPushButton*> plusButtons, minusButtons, homeButtons;
    QVector<QPushButton*> originButtons;
    QVector<QLabel*> axisPositionLabels;
    QComboBox *stepSizeCombo;
    QComboBox *axisGroupCombo;
    QSlider *overrideSlider;
    QLabel *overrideLabel;
    int axisCount;
    int axisPageSize;
};

class AlarmPanel : public QWidget {
    Q_OBJECT
public:
    AlarmPanel(QWidget *parent = nullptr);
    void addAlarm(const QString &message, const QString &timestamp);
    void clearAlarms();

private:
    QTableWidget *alarmTable;
    QTextEdit *logDisplay;
};

class MainWindowRefactored : public QMainWindow {
    Q_OBJECT

public:
    MainWindowRefactored(QWidget *parent = nullptr);
    ~MainWindowRefactored();

private slots:
    // Dashboard
    void updateGlobalStatus();
    void updateCommunicationStatus();
    
    // Jogging
    void onJogPressed(int axis, int direction);
    void onJogReleased(int axis);
    void onStepSizeChanged(double size);
    void onOverrideChanged(int percent);
    void onHomeRequested(int axis);
    void onHomeAllRequested();
    void onSetCurrentAsOriginRequested(int axis);
    
    // Timer
    void onUpdateTimer();
    
    // ZMQ
    void onZMQConnected();
    void onZMQDisconnected();
    void onZMQError(const QString &error);
    void onConnectClicked();

    // Status
    void onAxisPositionsUpdated(QVector<double> positions);
    void onTaskSchedulingUpdated(const QString &state);

private:
    void setupUI();
    void setupConnections();
    void setupStyles();
    void createDashboard();
    void createPositionDisplay();
    void createJogControl();
    void createTrajectoryPanel();
    void createAlarmPanel();
    void createSettingsPanel();
    void createAdvancedModules();
    void createQuickActions();

    // Quick action helper
    QPushButton* addQuickAction(const QString &text, const QString &iconPath = QString());
    
    // UI Components
    StatusIndicator *globalStatus;
    QLabel *zmqStatusLabel, *etherCATStatusLabel;
    QLabel *homedLabel, *servoLabel, *schedStateLabel;
    QLineEdit *ipInput;
    QPushButton *connectBtn;
    
    QVector<AxisPositionDisplay*> axisDisplays;
    JogControlPanel *jogPanel;
    QGroupBox *quickActionGroup;
    QGridLayout *quickActionLayout;
    AlarmPanel *alarmPanel;
    
    // Advanced Modules
    QTabWidget *advancedTabs;
    TrajectoryPanel *trajectoryPanel;
    BehaviorTreePanel *behaviorTreePanel;
    CommandPanel *commandPanel;
    
    // Backend
    NRTProcess *nrtProcess;
    ZMQClient *zmqClient;
    ZMQStatusSubscriber *statusSubscriber;
    QTimer *updateTimer;
    
    // State
    bool useZMQ;
    double currentOverride;
    double currentStepSize;
    
    // Helper methods
    void sendMotionCommand(const QString &command, const QVector<double> &args = {});
    void showConfirmDialog(const QString &title, const QString &message, std::function<void()> onConfirm);
    void feedPoseToTrajectory();
};
