.PHONY: module app clean test
obj-m += creme.o
PROG = module app
TEST = test

all: $(PROG)
module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
app:
	gcc -o $(TEST) $(TEST).c
clean:
	(rm -rf *.o *.ko modules.order creme.mod.c Module.symvers creme.mod $(TEST))
test:
	sudo insmod creme.ko
	sudo ./$(TEST)
	sudo rmmod creme.ko
