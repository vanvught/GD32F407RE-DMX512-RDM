#!/bin/bash
PATH=".:./../scripts:$PATH"

if [ -f $2 ]; then
echo $2
else
exit
fi

echo '!tftp#1' | udp_send $1 
ON_LINE=$(echo '?tftp#' | udp_send $1 ) || true
echo [$ON_LINE]

while  [ "$ON_LINE" == "tftp:Off" ]
 do
    sleep 1
    echo '!tftp#1' | udp_send $1 
    ON_LINE=$(echo '?tftp#' | udp_send $1 )  || true
done

sleep 1
echo -e "Rebooting..."
echo '?reboot##' | udp_send $1 

ON_LINE=$(echo '?list#' | udp_send $1 ) || true

while  [ "$ON_LINE" == "" ]
 do
  ON_LINE=$(echo '?list#' | udp_send $1 )  || true
done

echo '!tftp#1' | udp_send $1 
echo '?tftp#' | udp_send $1 

tftp $1 << -EOF
binary
put $2
quit
-EOF

echo '!tftp#0' | udp_send $1 
sleep 1
echo '?tftp#' | udp_send $1 
sleep 2
echo -e "Rebooting..."
echo '?reboot##' | udp_send $1 

ON_LINE=$(echo '?list#' | udp_send $1 ) || true

while  [ "$ON_LINE" == "" ]
 do
  ON_LINE=$(echo '?list#' | udp_send $1 )  || true
done

echo -e "[$ON_LINE]"
echo -e "$(echo '?version#' | udp_send $1 )"
