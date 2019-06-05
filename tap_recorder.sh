#!/bin/bash
# 在WSL环境下测试，WSL下使用lsusb无法检测到手机，adb指令无法使用，因此改用Windows下的adb
# 如需在Linux下测试，将./adb.exe改为adb即可
EX=./adb.exe
# 屏幕宽度
width=1080

if [ $# = 2 ]; then
	if [ $1 = "-r" ]; then
		record=true
	else
		if [ ! -f "$1" ]; then 
			echo $1: 没有找到该文件
			exit 0
		fi
		if echo $2 | grep -q '[^0-9]'; then
			echo $2: 不是一个数字
			exit 0
		fi
		record=false
	fi
else
	echo "Usage: $0 <<filename> <time>> | <-r <filename>>"
	echo fliename: 要载入/记录的文件名
	echo -r: 记录点击坐标到文件
	echo time: 要总共要执行的次数
	exit 0
fi

if [ record = true ]; then
	echo 记录到$2
	echo 开始记录，按Ctrl+C退出
	j=1
	while true
	do
		$EX shell getevent -c 14 -t | grep -e "0035" -e "0036" >> $2
		echo "记录第$j次点击..."
		let "j++"
	done
else
	echo 当前是横屏模式，读取$1文件，执行$2次

	for j in {1..$2}
	do
		sleep 5
		last_time=0
		all=`awk '{print NR}' $1 | tail -n1`
		
		i=1
		while [ $i -le $all ]
		do
			line=`sed -n ${i}p $1`
			time1=${line:1:15}
			type1=${line:44:2}
			value1=${line:47:8}
			#echo $time1 $type1 $value1
			let "i++"
			line=`sed -n ${i}p $1`
			time2=${line:1:15}
			type2=${line:44:2}
			value2=${line:47:8}
			#echo $time2 $type2 $value2
			
			if [ $time1 = $time2 ]; then
				let "i++"
				if [ $type1 = 35 ]; then
					let x=16#$value1
					let y=16#$value2
				else
					let x=16#$value2
					let y=16#$value1
				fi

				if [ $last_time != 0 ]; then
					#echo $time1 $last_time
					inv=$( echo "scale=6; $time1-$last_time" | bc)
				else
					inv=0
				fi
				echo 休眠$inv秒
				sleep $inv
				# 横屏模式
				echo 点击$y $[$width-$x]
				$EX shell input tap $y $[$width-$x]

				last_time=$time1
			else
				echo 发现无效记录点$time1，跳过
			fi
		done
	done
fi