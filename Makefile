# Makefile for keypad16 device driver

BASE_DIR=..
LKM_DIR=$(BASE_DIR)/linux/linux-4.13.10
#LKM_DIR=/lib/modules/$(shell uname -r)/build
#LKM_DIR=/usr/src/kernels/4.1.10-200.fc22.x86_64
BUILDROOT_HOME=$(BASE_DIR)/buildroot/buildroot-2017.02.7
CCC=$(BUILDROOT_HOME)/output/host/usr/bin/i486-linux-gcc
CCFLAGS=--sysroot=$(BUILDROOT_HOME)/output/staging

#-m32 -march=i486

#sources = $(wildcard keypad16_a71[0-5].c)

sources = keypad16_a710.c keypad16_a711.c keypad16_a712.c keypad16_a713.c keypad16_a714.c keypad16_a715.c keypad16_a716.c keypad16_a717.c

obj-m += $(sources:.c=.o)

modules =  $(sources:.c=.ko)

LKM_DIR=/home/jml/arcom/karcom/linux-4.13.10

utils = tcode tkey tcode-static tkey-static

all: $(modules) $(utils)

%.ko: %.c
	make -C ${LKM_DIR} M=$(PWD) modules

tcode: tcode.c
	$(CCC) -Wall $(CCFLAGS) -o $@ $^

tkey: tkey.c
	$(CCC) -Wall $(CCFLAGS) -o $@ $^

tcode-static: tcode.c
	cc -Wall  -m32 -march=i486 -static -o $@ $^

tkey-static: tkey.c
	cc -Wall  -m32 -march=i486 -static -o $@ $^

clean:
	make -C ${LKM_DIR} M=$(PWD) clean
	rm -f tcode tkey


