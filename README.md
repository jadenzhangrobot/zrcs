zrcs (zhang real time control system)
# 需要先安装JZMQ绑定 linxu系统
sudo apt install libzmq3-dev
sudo apt install nlohmann-json3-dev
sudo apt install libeigen3-dev
sudo apt install libabsl-dev
sudo apt install qt6-multimedia-dev
sudo apt install libqt6svg6-dev qt6-base-dev
sudo apt install libncurses-dev libncursesw5-dev

#windos系统msys2系统安装
pacman -S mingw-w64-ucrt-x86_64-nlohmann-json
pacman -S mingw-w64-ucrt-x86_64-zeromq
pacman -S mingw-w64-ucrt-x86_64-cppzmq
pacman -S mingw-w64-ucrt-x86_64-protobuf
pacman -S mingw-w64-ucrt-x86_64-opencascade
pacman -S mingw-w64-ucrt-x86_64-eigen3

编译器要求gcc 9.3
需要库ruckig
ethercat_rtdm

cmake -Drealtime=YES -Ddebug=YES -Dethercat=YES ..


 sudo ufw allow 5555
 sudo ufw allow 5556


# Windows 打包安装包教程

当前仓库已经提供 Windows 安装包脚本：

`tool/package/package_installer.sh`

这个脚本会自动完成下面几件事：

- 复制 `build/bin/zrcsgui.exe`
- 附带复制 `motiongui.exe`、`zrcsnrt.exe`、`zrcsrt.exe`
- 使用 `windeployqt6` 收集 Qt 依赖
- 使用 `ldd` 递归补齐 MSYS2 运行时 DLL
- 生成 NSIS 安装包 `.exe`

## 1. 环境要求

Windows 打包必须使用 MSYS2 UCRT64 环境。

至少需要安装这些包：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
pacman -S mingw-w64-ucrt-x86_64-qt6-svg
pacman -S mingw-w64-ucrt-x86_64-qt6-multimedia
pacman -S mingw-w64-ucrt-x86_64-zeromq
pacman -S mingw-w64-ucrt-x86_64-cppzmq
pacman -S mingw-w64-ucrt-x86_64-protobuf
pacman -S mingw-w64-ucrt-x86_64-abseil-cpp
pacman -S mingw-w64-ucrt-x86_64-nsis
```

脚本运行时还会自动检查：

- `windeployqt6`
- `makensis`
- `ldd`

如果缺少，会直接报错并提示需要安装的包。

## 2. 先编译要打包的程序

先在项目根目录编译至少这些目标：

```powershell
cmake --build build --target zrcsgui --config Release
cmake --build build --target motiongui --config Release
cmake --build build --target zrcsnrt --config Release
cmake --build build --target zrcsrt --config Release
```

编译完成后，下面这些文件应该存在：

- `build/bin/zrcsgui.exe`
- `build/bin/motiongui.exe`
- `build/bin/zrcsnrt.exe`
- `build/bin/zrcsrt.exe`

## 3. 运行打包脚本

推荐在项目根目录运行。

### 方式一：PowerShell 调用 MSYS2 bash

```powershell
D:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
```

如果你的 MSYS2 安装在 `C:\msys2`，就改成：

```powershell
C:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
```

### 方式二：在 Git Bash 或 MSYS2 bash 中运行

```bash
bash tool/package/package_installer.sh
```

也可以先进入脚本目录再执行：

```bash
cd tool/package
bash ./package_installer.sh
```

## 4. 打包输出位置

脚本成功后会生成：

- 安装包：`build/ZRCS-2.0.0-Setup.exe`
- 临时收集目录：`build/installer_staging/`

其中：

- `build/installer_staging/` 是打包中间目录
- `build/ZRCS-2.0.0-Setup.exe` 是最终给用户运行的安装包

## 5. 常见问题

### 1）提示找不到 `zrcsgui.exe`

说明主 GUI 还没有编译成功，先执行：

```powershell
cmake --build build --target zrcsgui --config Release
```

### 2）提示找不到 `windeployqt6`

说明 Qt6 基础包没有装好，执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
```

### 3）提示找不到 `makensis`

说明 NSIS 没装，执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-nsis
```

### 4）安装包生成成功，但想确认包含哪些程序

当前安装包默认包含：

- `zrcsgui.exe`
- `motiongui.exe`
- `zrcsnrt.exe`
- `zrcsrt.exe`

如果后续还要加入别的可执行文件，可以修改：

- `tool/package/package_installer.sh` 中的 `EXTRA_EXES`







