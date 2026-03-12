@echo off
REM 创建文件夹结构
mkdir src\core
mkdir src\components
mkdir src\communication
mkdir src\config
mkdir modules\trajectory
mkdir modules\gcode
mkdir modules\remote
mkdir modules\plugin
mkdir resources\ui
mkdir resources\style
mkdir resources\docs

REM 移动源代码文件到 src/core
move main_refactored.cpp src\core\ 2>nul
move mainwindow_refactored.h src\core\ 2>nul
move mainwindow_refactored.cpp src\core\ 2>nul

REM 移动组件文件到 src/components
move statusIndicator.cpp src\components\ 2>nul
move jogAndIOPanel.cpp src\components\ 2>nul
move alarmPanel.cpp src\components\ 2>nul

REM 移动通信文件到 src/communication
move zmqClient.h src\communication\ 2>nul
move zmqClient.cpp src\communication\ 2>nul

REM 移动配置文件到 src/config
move zrcsConfig.h src\config\ 2>nul
move zrcsStyles.h src\config\ 2>nul

REM 移动 UI 文件到 resources/ui
move ui\mainwindow_refactored.ui resources\ui\ 2>nul

REM 移动样式文件到 resources/style
move style\dark_theme.qss resources\style\ 2>nul
move style\styleLoader.h resources\style\ 2>nul

REM 移动文档文件到 resources/docs
move QUICK_START.md resources\docs\ 2>nul
move GUI_REFACTOR.md resources\docs\ 2>nul
move ARCHITECTURE.md resources\docs\ 2>nul
move ADVANCED_FEATURES.md resources\docs\ 2>nul
move UI_STYLE_GUIDE.md resources\docs\ 2>nul
move CODE_GENERATION_GUIDE.md resources\docs\ 2>nul
move CODE_GENERATION_COMPLETE.md resources\docs\ 2>nul
move FOLDER_STRUCTURE.md resources\docs\ 2>nul
move FOLDER_STRUCTURE_SUMMARY.md resources\docs\ 2>nul
move PROJECT_COMPLETION_SUMMARY.md resources\docs\ 2>nul
move SUMMARY.md resources\docs\ 2>nul
move FILE_ORGANIZATION_PLAN.md resources\docs\ 2>nul
move FILE_ORGANIZATION_GUIDE.md resources\docs\ 2>nul

REM 移动模块文件
move trajectory\trajectoryVisualizer.h modules\trajectory\ 2>nul
move trajectory\trajectoryVisualizer.cpp modules\trajectory\ 2>nul
move gcode\gcodeEditor.h modules\gcode\ 2>nul
move gcode\gcodeEditor.cpp modules\gcode\ 2>nul
move remote\remoteMonitor.h modules\remote\ 2>nul
move remote\remoteMonitor.cpp modules\remote\ 2>nul
move plugin\pluginInterface.h modules\plugin\ 2>nul
move plugin\pluginManager.h modules\plugin\ 2>nul
move plugin\pluginManager.cpp modules\plugin\ 2>nul

echo.
echo ========================================
echo 文件整理完成！
echo ========================================
echo.
echo 新的目录结构：
echo src/
echo   ├── core/
echo   ├── components/
echo   ├── communication/
echo   └── config/
echo.
echo modules/
echo   ├── trajectory/
echo   ├── gcode/
echo   ├── remote/
echo   └── plugin/
echo.
echo resources/
echo   ├── ui/
echo   ├── style/
echo   └── docs/
echo.
pause
