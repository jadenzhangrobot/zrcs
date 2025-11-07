#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QTimer>
#include <functional>
#include "common/sharedMemory/nrt_process.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 控制按钮槽函数
    void onPlusXPressed();   // +X按钮按下
    void onPlusXReleased();  // +X按钮松开
    void onMinusXPressed();  // -X按钮按下
    void onMinusXReleased(); // -X按钮松开
    void onPlusYPressed();   // +Y按钮按下
    void onPlusYReleased();  // +Y按钮松开
    void onMinusYPressed();  // -Y按钮按下
    void onMinusYReleased(); // -Y按钮松开
    void onPlusZPressed();   // +Z按钮按下
    void onPlusZReleased();  // +Z按钮松开
    void onMinusZPressed();  // -Z按钮按下
    void onMinusZReleased(); // -Z按钮松开
    void onPlusAlphaPressed();   // +α按钮按下
    void onPlusAlphaReleased();  // +α按钮松开
    void onMinusAlphaPressed();  // -α按钮按下
    void onMinusAlphaReleased(); // -α按钮松开
    void onPlusBetaPressed();    // +β按钮按下
    void onPlusBetaReleased();   // +β按钮松开
    void onMinusBetaPressed();   // -β按钮按下
    void onMinusBetaReleased();  // -β按钮松开
    void onPlusXClicked();   // 保留原有的clicked槽函数
    void onMinusXClicked();
    void onPlusYClicked();
    void onMinusYClicked();
    void onPlusZClicked();
    void onMinusZClicked();
    void onPlusAlphaClicked();
    void onMinusAlphaClicked();
    void onPlusBetaClicked();
    void onMinusBetaClicked();
    
    // 回零按钮槽函数
    void onXAxisHomeClicked();
    void onYAxisHomeClicked();
    void onZAxisHomeClicked();
    void onAlphaAxisHomeClicked();
    void onBetaAxisHomeClicked();
    void onAllAxisHomeClicked();
    
    // 定时器槽函数
    void onTimerTimeout();

private:
    Ui::MainWindow *ui;
    NRTProcess *nrt_process;
    QTimer *moveTimer;  // 用于长按移动的定时器
    std::function<void()> currentMoveFunction;  // 当前活动的移动函数
    // 初始化信号槽连接
    void setupConnections();
    
    // 显示状态信息
    void showStatusMessage(const QString &message);
};

#endif // MAINWINDOW_H