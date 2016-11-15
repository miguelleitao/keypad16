# Makefile for keypad16 device driver

#sources = $(wildcard keypad16_a[0-4].c)
sources = keypad16_a710.c keypad16_a711.c keypad16_a712.c keypad16_a713.c keypad16_a714.c keypad16_a715.c keypad16_a717.c

obj-m += $(sources:.c=.o)

mods =  $(sources:.c=.ko)

#LKM_DIR=/lib/modules/$(shell uname -r)/build
#LKM_DIR=/home/arq2/lab14/linux-2.6.21.1
LKM_DIR=/home/jml/kit/linux-2.6.29.6
LKM_DIR=/home/jml/kit/linux-2.6.34.14

all: $(mods)

%.ko: %.c
	make -C ${LKM_DIR} M=$(PWD) modules

clean:
	make -C ${LKM_DIR} M=$(PWD) clean


