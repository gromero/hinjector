CC=gcc
obj-m += k.o

all:	injector
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

insmod:	k.ko
	sudo insmod k.ko
	sudo mknod /dev/injector c 241 0

rmmod:	k.ko
	@sudo rmmod k.ko

injection: payload.S
	gcc payload.S -c -o payload.o
	objcopy -O binary -j .text payload.o payload.bin
	./injection.sh

injector: injector.o
	gcc injector.c -o injector

clean:
	rm -fr k.ko k.mod.*
	rm -fr injector
	rm -fr *.o
