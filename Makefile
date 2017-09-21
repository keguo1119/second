KVERS = $(shell uname -r)

#Kernel modules
obj-m += second.o
obj-m += second_test.o
#obj-m += second_mutex.o
obj-m += second_dri.o
obj-m += second_dev.o

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

user_test:
	gcc test.c -o test

clean:
	rm *.ko *.mod.c  *.o 