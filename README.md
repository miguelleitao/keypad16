# keypad16
======
keypad16 is a device driver for small keyboards connected to the parallel port.

keypad16 is implemented as a Linux Kernel module and can be compiled out of the tree for most Kernel versions.

keypad16 is not aimed to production. Its goal is to provide programing examples and to help learning.

keypad16 is available in several steps that are not related to software versions.

### Steps

* keypad16_a710: basic device driver skeleton. Does not implement keyboard scanning.

* keypad16_a711: basic keyboard device driver. Keyboard scanning and output as scancode.

* keypad16_a712: Output as ASCII key code. Conversion from scancode to key is done by a static table.

* keypad16_a713: Added event detector.

* keypad16_a714: Added input buffer.

* keypad16_a715: Added /proc interface

### Sugested usage
```bash
# import all
git clone https://github.com/miguelleitao/keypad16.git
cd keypad16
ls
# compile all
make
ls
# try step 0
insmod keypad16_a710.ko
lsmod
dmesg
mknod /dev/keypad16 c 431 0
cat /dev/keypad16
rmmod keypad16_a710
# try step 1
# Connect keypad to parallel port
insmod keypad16_a711.ko
lsmod
dmesg
cat /dev/keypad16
# press some keypad keys
^c
cat /dev/keypad16 |./tcode
# press some keypad keys
^c
rmmod keypad16_a711
# try step 2
insmod keypad16_a712.ko
cat /dev/keypad16
# press some keypad keys
^c
rmmod keypad16_a712
# try step 3
insmod keypad16_a713.ko
cat /dev/keypad16
# press and hold some keypad keys
^c
rmmod keypad16_a713
# try step 4
insmod keypad16_a714.ko
# press some keypad keys
cat /dev/keypad16
# press some keypad keys
^c
rmmod keypad16_a714
# try step 5
insmod keypad16_a715.ko
cat /proc/keypad16/table
cat /proc/keypad16/rep_first
cat /proc/keypad16/rep_time
cat /proc/keypad16/buffer
# press some keypad keys
cat /proc/keypad16/buffer
cat /dev/keypad16
cat /proc/keypad16/buffer
# press and hold some keypad keys
^c
rmmod keypad16_a715
```
