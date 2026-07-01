port=/dev/ttyACM0

echo -e "to show output start: \nwhile true; do cat - < $port; sleep 1; done"

stty -F $port raw -echo

while true; do
       echo -ne "\xA0\x01\x01\xA2" > $port
       echo switched on relay 1
       sleep 1
       echo -ne "\xA0\x01\x00\xA1" > $port
       echo switched off relay 1
       sleep 1
done
