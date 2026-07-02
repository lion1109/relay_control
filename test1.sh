port=/dev/ttyACM0

# echo -e "to show output start: \nstty -F $port raw -echo; while true; do cat - < $port; sleep 1; done"

# stty -F $port raw -echo

# the stty command can be executed by systemd-udev if configured.
#   udev config file: /etc/udev/rules.d/99-esp32-serial.rules
#   rule in file:
#     ACTION=="add", SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", RUN+="/bin/stty -F /dev/%k raw -echo 115200" 
#
# reload and activate after configuration modifications:
#   sudo udevadm control --reload-rules
#   sudo udevadm trigger
#
# in windows wsl you can install usbipd and start accessing by:
# usbipd list
# usbipd bind --busid 2-1                       # once to bind
# usbipd attach --wsl --busid 2-1               # for connect after each plug in
# usbipd attach --wsl --busid 2-1 --auto-attach # for automatic reconnect

while true; do
       echo -ne "\xA0\x01\x01\xA2" > $port
       echo switched on relay 1
       sleep 1
       echo -ne "\xA0\x01\x00\xA1" > $port
       echo switched off relay 1
       sleep 1
done
