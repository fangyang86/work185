
# gpio7_0  status
echo 192 >/sys/class/gpio/unexport
echo 192 >/sys/class/gpio/export
# gpio7_1   config
echo 193 >/sys/class/gpio/unexport
echo 193 >/sys/class/gpio/export
# gpio7_2   data
echo 194 >/sys/class/gpio/unexport
echo 194 >/sys/class/gpio/export
# gpio7_6    clk
echo 198 >/sys/class/gpio/unexport
echo 198 >/sys/class/gpio/export
# gpio7_7     conf_done
echo 199 >/sys/class/gpio/unexport
echo 199 >/sys/class/gpio/export

echo in >/sys/class/gpio/gpio192/direction
echo out >/sys/class/gpio/gpio193/direction
echo out >/sys/class/gpio/gpio194/direction
echo out >/sys/class/gpio/gpio198/direction
echo in >/sys/class/gpio/gpio199/direction

rm -f status
rm -f config
rm -f data
rm -f clk
rm -f done

ln -s /sys/class/gpio/gpio192/value status
ln -s /sys/class/gpio/gpio193/value config
ln -s /sys/class/gpio/gpio194/value data
ln -s /sys/class/gpio/gpio198/value clk
ln -s /sys/class/gpio/gpio199/value done

ls -l
