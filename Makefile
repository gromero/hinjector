obj-m += k.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

insmod:	k.ko
	@sudo insmod k.ko

rmmod:	k.ko
	@sudo rmmod k.ko

injection: payload.S
	gcc payload.S -c -o payload.o
	objcopy -O binary -j .text payload.o payload.bin
	./injection.sh

clean:
	rm -fr k.ko k.mod.*
