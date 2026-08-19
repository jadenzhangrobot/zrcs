#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QScrollBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <functional>
#include "communication/ZmqClient.h"
#include "communication/ZmqStatusSubscriber.h"
#include "mujoco/MujocoVisualizer.h"
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

class JogControlPanel : public QWidget {
    Q_OBJECT
public:
    JogControlPanel(QWidget *parent = nullptr);
    void setAxisCount(int count);
    void setAxisPosition(int axis, double position);
    void setAxisServoStates(const QVector<quint8> &states);
    
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
    QVector<double> axisPositions;
    QVector<quint8> servoStates_;
    QComboBox *stepSizeCombo;
    QComboBox *axisGroupCombo;
    QScrollBar *axisScrollBar;
    QSlider *overrideSlider;
    QLabel *overrideLabel;
    int axisCount;
    int axisPageSize;
};

class AlarmPanel : public QWidget {
    Q_OBJECT
public:
    AlarmPanel(QWidget *parent = nullptr);
    void addAlarm(const QString &message, const QString &timestamp, const QString &type = QStringLiteral("错误"));
    void appendOperationLog(const QString &message);

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
    void onCommandPanelCommandRequested(const QString &cmd, const QVector<double> &args);

    // Status
    void onAxisPositionsUpdated(QVector<double> positions);
    void onAxisServoStatesUpdated(QVector<quint8> enabled);
    void onTaskSchedulingUpdated(const QString &state);
    void onRtLogReceived(quint32 level, const QString &message, const QString &timestamp);
    void onBtStatusUpdated(const QString &treeState,
                           const QString &currentNode,
                           const QString &message);

private:
    void setControlPanelExpanded(bool expanded);
    void setupUI();
    void setupConnections();
    void setupStyles();
    void createJogControl();
    void createMujocoPanel();
    void createAlarmPanel();
    void createSettingsPanel();
    void createAdvancedModules();
    void createQuickActions();
    void bindBehaviorTreeClient();
    
    // UI Components
    StatusIndicator *globalStatus;
    QLabel *zmqStatusLabel, *etherCATStatusLabel;
    QLabel *homedLabel, *servoLabel, *schedStateLabel;
    QLineEdit *ipInput;
    QPushButton *connectBtn;
    QWidget *controlSidebarHost;
    QScrollArea *controlScrollArea;
    QPushButton *controlPanelToggleButton;

    JogControlPanel *jogPanel;
    QGroupBox *quickActionGroup;
    AlarmPanel *alarmPanel;
    
    // Advanced Modules
    QTabWidget *advancedTabs;
    MujocoPanel *mujocoPanel;
    BehaviorTreePanel *behaviorTreePanel;
    CommandPanel *commandPanel;
    
    // Backend
    ZMQClient *zmqClient;
    ZMQStatusSubscriber *statusSubscriber;
    QTimer *updateTimer;
    
    // State
    bool useZMQ;
    bool controlPanelExpanded_;
    double currentOverride;
    double currentStepSize;
    
    // Helper methods
    void sendMotionCommand(const QString &command, const QVector<double> &args = {});
    void showConfirmDialog(const QString &title, const QString &message, std::function<void()> onConfirm);
};
