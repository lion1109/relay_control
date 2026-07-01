port=/dev/ttyACM0

echo -e "to show output start: \nwhile true; do cat - < $port; sleep 1; done"

stty -F $port raw -echo

echo -ne "\xA0\xFE\x00\x9E" > $port
echo set active level to low = 0 [V]

sleep 5

echo -ne "\xA0\xFE\x01\x9F" > $port
echo set active level to high = 3.3 [V]
