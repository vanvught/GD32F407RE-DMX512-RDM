#!/bin/bash
NPROC=1

if [ "$(uname)" == "Darwin" ]; then
     NPROC=$(sysctl -a | grep machdep.cpu.core_count | cut -d ':' -f 2)     
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
     NPROC=$(nproc)
fi

cd ..

DIR=gd32_*
MAKEFILE=Makefile*.GD32

array=('DP83848' 'RTL8201F' 'LAN8700')

for f in $DIR
do
	rm -rf /tmp/$f
	mkdir /tmp/$f
	for i in "${array[@]}"
	do
		mkdir /tmp/$f/$i
	done
done

for f in $DIR
do
	echo -e "\e[32m[$f]\e[0m"
	if [ -d $f ]; then
		cd "$f"	
		
		for i in "${array[@]}"
		do
		
			for m in $MAKEFILE
			do
				make -f $m -j $NPROC clean
				make -f $m -j $NPROC ENET_PHY=$i
				retVal=$?
				
				if [ $retVal -ne 0 ]; then
				 	echo "Error : " "$f" " : " "$m"
					exit $retVal
				fi
				
				SUFFIX=$(echo $m | cut -d '-' -f 2 | cut -d '.' -f 1)
			
				if [ $SUFFIX == 'Makefile' ]
				then
					cp gd32f4xx.bin /tmp/$f/$i
				else
					echo $SUFFIX
					mkdir /tmp/$f/$i/$SUFFIX/
					cp gd32f4xx.bin /tmp/$f/$i/$SUFFIX
				fi			
			done
			
		done
			
		cd -
	fi
done

find . -name gd32f4xx.bin | sort | xargs ls -al
find . -name gd32f4xx.bin | xargs ls -al | wc -l