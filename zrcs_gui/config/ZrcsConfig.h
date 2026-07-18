#pragma once

#include <QString>
#include <QVector>
#include <QSettings>

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
    int zmqMaxRetries = 10;            // 最大重试次数，0 表示无限
    int zmqBackoffMultiplier = 2;      // 指数退避倍率
    int zmqMaxReconnectIntervalMs = 30000; // 退避上限 (ms)

    // 共享内存
    QString shmName = "rtMotion";
};

// 运动控制配置（与控制器 SI 一致：直线 m / m/s；旋转 rad）
struct MotionConfig {
    // 点动步长选项 (m)。0 = 连续点动。
    // 对应界面标签 连续 / 10mm / 1mm / 0.1mm / 0.01mm。
    QVector<double> stepSizes = {0.0, 0.01, 0.001, 0.0001, 0.00001};

    // 默认倍率 (%)
    int defaultOverride = 100;
    int minOverride = 0;
    int maxOverride = 100;

    // 速度限制 (m/s)
    double maxVelocity = 1.0;
    double maxAcceleration = 0.5;
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

    // 软限位 (m)，示教用宽松范围
    double softLimitMin = -1.0;
    double softLimitMax = 1.0;
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
    enum Level { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_CRITICAL };
    Level minLevel = LOG_INFO;
    
    // 输出
    bool logToFile = true;
    QString logFilePath = "./logs/zrcsgui.log";
    bool logToConsole = true;
    
    // 格式
    QString dateFormat = "yyyy-MM-dd hh:mm:ss";
    bool includeTimestamp = true;
    bool includeLevel = true;
};

// 全局配置实例（支持持久化到文件）
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

    // 从文件加载配置，不存在则保持默认值
    void load(const QString& path = "./config/zrcsgui.ini") {
        QSettings s(path, QSettings::IniFormat);

        // UI
        s.beginGroup("UI");
        ui.windowWidth = s.value("windowWidth", ui.windowWidth).toInt();
        ui.windowHeight = s.value("windowHeight", ui.windowHeight).toInt();
        ui.updateIntervalMs = s.value("updateIntervalMs", ui.updateIntervalMs).toInt();
        ui.axisCount = s.value("axisCount", ui.axisCount).toInt();
        ui.inputCount = s.value("inputCount", ui.inputCount).toInt();
        ui.outputCount = s.value("outputCount", ui.outputCount).toInt();
        s.endGroup();

        // Comm
        s.beginGroup("Comm");
        comm.zmqHost = s.value("zmqHost", comm.zmqHost).toString();
        comm.zmqPort = s.value("zmqPort", comm.zmqPort).toInt();
        comm.zmqTimeoutMs = s.value("zmqTimeoutMs", comm.zmqTimeoutMs).toInt();
        comm.zmqAutoReconnect = s.value("zmqAutoReconnect", comm.zmqAutoReconnect).toBool();
        comm.zmqMaxRetries = s.value("zmqMaxRetries", comm.zmqMaxRetries).toInt();
        s.endGroup();

        // Motion
        s.beginGroup("Motion");
        motion.defaultOverride = s.value("defaultOverride", motion.defaultOverride).toInt();
        motion.maxVelocity = s.value("maxVelocity", motion.maxVelocity).toDouble();
        motion.maxAcceleration = s.value("maxAcceleration", motion.maxAcceleration).toDouble();
        s.endGroup();

        // Safety
        s.beginGroup("Safety");
        safety.softLimitMin = s.value("softLimitMin", safety.softLimitMin).toDouble();
        safety.softLimitMax = s.value("softLimitMax", safety.softLimitMax).toDouble();
        safety.confirmHoming = s.value("confirmHoming", safety.confirmHoming).toBool();
        safety.confirmEStop = s.value("confirmEStop", safety.confirmEStop).toBool();
        s.endGroup();
    }

    // 保存当前配置到文件
    void save(const QString& path = "./config/zrcsgui.ini") {
        QSettings s(path, QSettings::IniFormat);

        s.beginGroup("UI");
        s.setValue("windowWidth", ui.windowWidth);
        s.setValue("windowHeight", ui.windowHeight);
        s.setValue("updateIntervalMs", ui.updateIntervalMs);
        s.setValue("axisCount", ui.axisCount);
        s.setValue("inputCount", ui.inputCount);
        s.setValue("outputCount", ui.outputCount);
        s.endGroup();

        s.beginGroup("Comm");
        s.setValue("zmqHost", comm.zmqHost);
        s.setValue("zmqPort", comm.zmqPort);
        s.setValue("zmqTimeoutMs", comm.zmqTimeoutMs);
        s.setValue("zmqAutoReconnect", comm.zmqAutoReconnect);
        s.setValue("zmqMaxRetries", comm.zmqMaxRetries);
        s.endGroup();

        s.beginGroup("Motion");
        s.setValue("defaultOverride", motion.defaultOverride);
        s.setValue("maxVelocity", motion.maxVelocity);
        s.setValue("maxAcceleration", motion.maxAcceleration);
        s.endGroup();

        s.beginGroup("Safety");
        s.setValue("softLimitMin", safety.softLimitMin);
        s.setValue("softLimitMax", safety.softLimitMax);
        s.setValue("confirmHoming", safety.confirmHoming);
        s.setValue("confirmEStop", safety.confirmEStop);
        s.endGroup();

        s.sync();
    }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

}  // namespace ZrcsConfig

