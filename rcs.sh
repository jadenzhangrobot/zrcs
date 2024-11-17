#!/bin/bash
rm plot.dat
i=0;
last_cnts=0;
cur_cnts=0;
while [ 1 ]
do 
	let i=i+1;
#获取当前中断数量
	cur_cnts=`cat /proc/interrupts | grep -rin 'gyro' | awk  '{print $3}'`
#第一次执行时，last_cnts为0，无需统计差值
	if [ $last_cnts -eq 0 ];then
		last_cnts=$cur_cnts;
		continue;
	fi
#获取差值，并按照"时间\t差值"的各执写入数据文件 plot.dat
	echo -e `date "+%H:%M:%S"`"\t"`expr ${cur_cnts} - ${last_cnts}` >> plot.dat;
	last_cnts=$cur_cnts;
#清除整个串口，以保证图表显示在窗口前端
	echo -e  "\033c"
#执行gnuplot 具体的配置就在 liveplot.gnu
	gnuplot  liveplot.gnu
#由于绘图操作需要消耗时间，因此1秒的延时并不准确，这才造成了统计的频率比设置的频率要高
	sleep 1
done

