obj-m+=TODO_LKM.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
	$(CC) TODO_USER.c -o test
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	rm test