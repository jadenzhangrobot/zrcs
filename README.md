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






