#Reference: http://www.devdrv.co.jp/linux/kernel26-makefile.htm
TARGET:= morse.ko

all: ${TARGET}

morse.ko: morse.c
#	make -C /usr/src/linux-headers-`uname -r` M=`pwd` V=1 modules
	make -C /usr/src/linux-headers-4.9.59+ M=`pwd` V=1 modules

clean:
#	make -C /usr/src/linux-headers-`uname -r` M=`pwd` V=1 clean
	make -C /usr/src/linux-headers-4.9.59+ M=`pwd` V=1 clean

obj-m:= morse.o

clean-files := *.o *.ko *.mod.[co] *~

install: all
	sudo mknod /dev/morse c 0x0123 0
	sudo chmod 0666 /dev/morse
	sudo insmod ./morse.ko
 
uninstall:
	sudo rmmod ./morse
	sudo rm -f /dev/morse
