# Makefile for keypad16 device driver

#sources = $(wildcard keypad16_a71[0-5].c)

sources = keypad16_a710.c keypad16_a711.c 

#keypad16_a712.c keypad16_a713.c keypad16_a714.c keypad16_a715.c
# keypad16_a717.c

obj-m += $(sources:.c=.o)

mods =  $(sources:.c=.ko)

LKM_DIR=/lib/modules/$(shell uname -r)/build
#LKM_DIR=/usr/src/kernels/4.1.10-200.fc22.x86_64

all: $(mods) tcode tkey

%.ko: %.c
	make -C ${LKM_DIR} M=$(PWD) modules

tcode: tcode.c
	cc -Wall -o $@ $^

tkey: tkey.c
	cc -Wall -o $@ $^

clean:
	make -C ${LKM_DIR} M=$(PWD) clean
	rm tcode tkey


