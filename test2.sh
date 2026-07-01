port=/dev/ttyACM0

echo -e "to show output start: \nwhile true; do cat - < $port; sleep 1; done"

stty -F $port raw -echo

while true; do
       echo -ne "\xA0\xFF\x01\xA0" > $port
       echo switched on all relays
       sleep 1
       echo -ne "\xA0\xFF\x00\x9F" > $port
       echo switched off all relays
       sleep 1
done
