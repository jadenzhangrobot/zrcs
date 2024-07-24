zrcs (zhang real time control system)
编译器要求gcc 9.3
需要库ruckig
ethercat_rtdm

<<<<<<< HEAD
cmake -D=realtime YES -D=debug NO -D=ethercat NO
=======
cmake -Drealtime=YES -Ddebug=YES -Dethercat=YES ..
>>>>>>> 9b4e769301abc65830cd5b869d35ea456004ed89
