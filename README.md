zrcs (zhang real time control system)
# 需要先安装JZMQ绑定 linxu系统
sudo apt install libjzmq-dev
sudo apt install nlohmann-json3-dev

#windos系统msys2系统安装
pacman -S mingw-w64-ucrt-x86_64-nlohmann-json
pacman -S mingw-w64-ucrt-x86_64-zeromq
pacman -S mingw-w64-ucrt-x86_64-cppzmq
pacman -S mingw-w64-ucrt-x86_64-protobuf


编译器要求gcc 9.3
需要库ruckig
ethercat_rtdm

cmake -Drealtime=YES -Ddebug=YES -Dethercat=YES ..






