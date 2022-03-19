#!/bin/bash
PATH=".:./../scripts:$PATH"
echo $1

DIR=../gd32_emac*
BIN=gd32f4xx.bin

array=('rconfig.txt' 'display.txt' 'network.txt' 'artnet.txt' 'e131.txt' 'devices.txt' 'params.txt')

for f in $DIR
do
	echo -e "[$f]"
	if [ -d $f ]; then
		cd $f
		if [ -f $BIN ]; then
			./../scripts/do-tftp.sh $1 $BIN
			
			for i in "${array[@]}"
			do
				echo $i
				TXT_FILE=$(echo ?get#$i | udp_send $1 )  || true
				echo $TXT_FILE
			done
		fi
		cd - >/dev/null
	fi
done
