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

git clone https://gitbub.org/miguelleitao/keypad16
cd keypad16
ls
make
ls
insmod keypad16_a710.ko
lsmod
dmesg
mknod /dev/keypad16 c 431 0
cat /dev/keypad16
rmmod keypad16_a710
#
# Connect keypad to parallel port
insmod keypad16_a710.ko
lsmod
dmesg
mknod /dev/keypad16 c 431 0
cat /dev/keypad16
^c
# press any keypad key
cat /dev/keypad16 |./tcode
# press any keypad key
^c
rmmod keypad16_a710
