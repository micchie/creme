obj-m += creme.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	(rm -rf *.o *.ko modules.order creme.mod.c Module.symvers creme.mod)
