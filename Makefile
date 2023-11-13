# Makefile for keypad16 device driver

#sources = $(wildcard keypad16_a71[0-5].c)

sources = keypad16_a710.c keypad16_a711.c keypad16_a712.c keypad16_a713.c keypad16_a714.c keypad16_a715.c
# keypad16_a717.c

obj-m += $(sources:.c=.o)

mods =  $(sources:.c=.ko)

#LKM_DIR=/lib/modules/$(shell uname -r)/build
#LKM_DIR=/usr/src/kernels/4.1.10-200.fc22.x86_64

LKM_DIR=/home/jml/arcom/karcom/linux-4.13.10

all: $(mods) tcode tkey tcode-static tkey-static

%.ko: %.c
	make -C ${LKM_DIR} M=$(PWD) modules

tcode: tcode.c
	cc -Wall  -m32 -march=i486 -o $@ $^

tkey: tkey.c
	cc -Wall  -m32 -march=i486 -o $@ $^

tcode-static: tcode.c
	cc -Wall  -m32 -march=i486 -static -o $@ $^

tkey-static: tkey.c
	cc -Wall  -m32 -march=i486 -static -o $@ $^

clean:
	make -C ${LKM_DIR} M=$(PWD) clean
	rm tcode tkey


