obj-m += k.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

insmod:	k.ko
	@sudo insmod k.ko

rmmod:	k.ko
	@sudo rmmod k.ko

clean:
	rm -fr k.ko k.mod.*
