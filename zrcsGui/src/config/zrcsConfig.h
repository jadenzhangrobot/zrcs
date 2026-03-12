#ifndef ZRCS_CONFIG_H
#define ZRCS_CONFIG_H

#include <QString>

namespace ZrcsConfig {

// UI 配置
struct UIConfig {
    // 窗口
    int windowWidth = 1600;
    int windowHeight = 1000;
    QString windowTitle = "ZRCS 工业运动控制系统 v2.0";
    
    // 更新频率
    int updateIntervalMs = 100;  // 100ms = 10Hz
    
    // 左侧面板宽度
    int leftPanelWidth = 500;
    
    // 轴数量
    int axisCount = 5;
    
    // I/O 数量
    int inputCount = 8;
    int outputCount = 6;
    
    // 日志
    int maxLogLines = 1000;
    int maxAlarmRows = 100;
};

// 通信配置
struct CommConfig {
    // ZMQ
    QString zmqHost = "localhost";
    int zmqPort = 5555;
    int zmqTimeoutMs = 5000;
    bool zmqAutoReconnect = true;
    int zmqReconnectIntervalMs = 1000;
    
    // 共享内存
    QString shmName = "rtMotion";
};

// 运动控制配置
struct MotionConfig {
    // 步长选项 (mm)
    QVector<double> stepSizes = {0, 10, 1, 0.1, 0.01};
    
    // 默认倍率 (%)
    int defaultOverride = 100;
    int minOverride = 0;
    int maxOverride = 100;
    
    // 速度限制 (mm/s)
    double maxVelocity = 1000;
    double maxAcceleration = 500;
};

// 安全配置
struct SafetyConfig {
    // 二次确认操作
    bool confirmHoming = true;
    bool confirmEStop = true;
    bool confirmReset = true;
    
    // 超时时间 (ms)
    int homingTimeoutMs = 30000;
    int commandTimeoutMs = 5000;
    
    // 软限位 (mm)
    double softLimitMin = -1000;
    double softLimitMax = 1000;
};

// 显示配置
struct DisplayConfig {
    // 坐标显示精度
    int positionPrecision = 3;      // 小数点后位数
    int velocityPrecision = 2;
    int accelerationPrecision = 2;
    int torquePrecision = 1;
    int temperaturePrecision = 1;
    
    // 刷新率
    bool enableAnimation = true;
    int animationDurationMs = 300;
    
    // 字体
    QString fontFamily = "Arial";
    int fontSizeNormal = 10;
    int fontSizeTitle = 12;
    int fontSizeLarge = 14;
};

// 日志配置
struct LogConfig {
    // 日志级别
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };
    Level minLevel = INFO;
    
    // 输出
    bool logToFile = true;
    QString logFilePath = "./logs/zrcsgui.log";
    bool logToConsole = true;
    
    // 格式
    QString dateFormat = "yyyy-MM-dd hh:mm:ss";
    bool includeTimestamp = true;
    bool includeLevel = true;
};

// 全局配置实例
class Config {
public:
    static Config& instance() {
        static Config config;
        return config;
    }
    
    UIConfig ui;
    CommConfig comm;
    MotionConfig motion;
    SafetyConfig safety;
    DisplayConfig display;
    LogConfig log;
    
private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

}  // namespace ZrcsConfig

#endif // ZRCS_CONFIG_H
