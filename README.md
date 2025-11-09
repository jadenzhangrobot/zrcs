zrcs (zhang real time control system)
# 需要先安装JZMQ绑定
sudo apt install libjzmq-dev
sudo apt install nlohmann-json3-dev

编译器要求gcc 9.3
需要库ruckig
ethercat_rtdm

cmake -Drealtime=YES -Ddebug=YES -Dethercat=YES ..
cmake -Drealtime=YES -Ddebug=NO -Dethercat=YES ..