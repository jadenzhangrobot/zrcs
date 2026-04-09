#ifndef ZRCS_STYLES_H
#define ZRCS_STYLES_H

#include <QString>

namespace ZrcsStyles {

// 颜色定义
namespace Colors {
    // 背景色
    constexpr const char* BACKGROUND_DARK = "#0f0f0f";
    constexpr const char* BACKGROUND_MAIN = "#1a1a1a";
    constexpr const char* BACKGROUND_PANEL = "#2a2a2a";
    
    // 文字色
    constexpr const char* TEXT_PRIMARY = "#CCCCCC";
    constexpr const char* TEXT_SECONDARY = "#999999";
    constexpr const char* TEXT_HIGHLIGHT = "#FFD700";
    
    // 状态色
    constexpr const char* STATUS_IDLE = "#64C864";      // 绿色
    constexpr const char* STATUS_RUNNING = "#6496FF";   // 蓝色
    constexpr const char* STATUS_ALARM = "#FFC800";     // 黄色
    constexpr const char* STATUS_ESTOP = "#FF5050";     // 红色
    
    // 数据显示色
    constexpr const char* DATA_POSITION = "#00FF00";    // 绿色
    constexpr const char* DATA_DYNAMICS = "#87CEEB";    // 蓝色
    constexpr const char* DATA_SERVO = "#FF6347";       // 红色
    
    // 边框色
    constexpr const char* BORDER_NORMAL = "#444444";
    constexpr const char* BORDER_HIGHLIGHT = "#FFD700";
    constexpr const char* BORDER_ERROR = "#FF6347";
}

// 样式表生成函数
inline QString getMainStyleSheet() {
    return QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { background-color: %2; color: %3; }"
        "QMenuBar { background-color: %4; color: %3; border-bottom: 1px solid %5; }"
        "QMenuBar::item:selected { background-color: %6; }"
        "QMenu { background-color: %4; color: %3; }"
        "QMenu::item:selected { background-color: %6; }"
        "QStatusBar { background-color: %4; color: %3; border-top: 1px solid %5; }"
        "QTabWidget::pane { border: 1px solid %5; }"
        "QTabBar::tab { background-color: %4; color: %3; padding: 8px 20px; border: 1px solid %5; }"
        "QTabBar::tab:selected { background-color: %6; color: %7; border-bottom: 2px solid %7; }"
        "QPushButton { background-color: %4; color: %3; border: 1px solid %8; border-radius: 3px; padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: %6; }"
        "QPushButton:pressed { background-color: %2; }"
        "QLineEdit { background-color: %4; color: %9; border: 1px solid %8; border-radius: 3px; padding: 5px; font-family: monospace; }"
        "QSpinBox { background-color: %4; color: %9; border: 1px solid %8; border-radius: 3px; padding: 5px; }"
        "QComboBox { background-color: %4; color: %3; border: 1px solid %8; border-radius: 3px; padding: 5px; }"
        "QLabel { color: %3; }"
        "QGroupBox { color: %7; border: 1px solid %5; border-radius: 5px; padding-top: 10px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
        "QSlider::groove:horizontal { background: %5; height: 8px; }"
        "QSlider::handle:horizontal { background: %9; width: 18px; margin: -5px 0; border-radius: 9px; }"
        "QTableWidget { background-color: %2; color: %3; border: 1px solid %5; }"
        "QHeaderView::section { background-color: %4; color: %7; padding: 5px; border: 1px solid %5; }"
        "QTableWidget::item { padding: 5px; border-bottom: 1px solid %10; }"
        "QTextEdit { background-color: %11; color: %9; border: 1px solid %5; font-family: monospace; font-size: 9pt; }"
        "QScrollArea { background-color: %2; border: none; }"
    )
    .arg(Colors::BACKGROUND_DARK)
    .arg(Colors::BACKGROUND_MAIN)
    .arg(Colors::TEXT_PRIMARY)
    .arg(Colors::BACKGROUND_PANEL)
    .arg(Colors::BORDER_NORMAL)
    .arg("#3a3a3a")
    .arg(Colors::TEXT_HIGHLIGHT)
    .arg("#555555")
    .arg(Colors::DATA_POSITION)
    .arg("#333333")
    .arg("#0a0a0a");
}

// 按钮样式
inline QString getButtonStyle(const QString& bgColor, const QString& textColor, const QString& borderColor) {
    return QString(
        "background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; padding: 8px; font-weight: bold;"
    ).arg(bgColor, textColor, borderColor);
}

// 绿色按钮（正常操作）
inline QString getGreenButtonStyle() {
    return getButtonStyle("#2a5a2a", Colors::DATA_POSITION, Colors::DATA_POSITION);
}

// 黄色按钮（警告操作）
inline QString getYellowButtonStyle() {
    return getButtonStyle("#5a5a2a", Colors::TEXT_HIGHLIGHT, Colors::TEXT_HIGHLIGHT);
}

// 红色按钮（危险操作）
inline QString getRedButtonStyle() {
    return getButtonStyle("#5a2a2a", Colors::DATA_SERVO, Colors::DATA_SERVO);
}

// 灰色按钮（禁用状态）
inline QString getGrayButtonStyle() {
    return getButtonStyle("#2a2a2a", Colors::TEXT_SECONDARY, Colors::BORDER_NORMAL);
}

// 标签样式
inline QString getLabelStyle(const QString& color) {
    return QString("color: %1; font-weight: bold;").arg(color);
}

// 数据显示标签
inline QString getDataLabelStyle() {
    return QString("color: %1; font-family: monospace; font-size: 11pt;").arg(Colors::DATA_POSITION);
}

// 分组框样式
inline QString getGroupBoxStyle(const QString& titleColor = Colors::TEXT_HIGHLIGHT) {
    return QString(
        "QGroupBox { color: %1; border: 1px solid %2; border-radius: 5px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
    ).arg(titleColor, Colors::BORDER_NORMAL);
}

// 高亮分组框
inline QString getHighlightGroupBoxStyle() {
    return QString(
        "QGroupBox { color: %1; border: 2px solid %1; border-radius: 5px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
    ).arg(Colors::TEXT_HIGHLIGHT);
}

}  // namespace ZrcsStyles

#endif // ZRCS_STYLES_H
