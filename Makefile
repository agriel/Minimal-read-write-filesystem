obj-m += hw9fs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mkcs385fs: mkcs385fs.c
	gcc -o mkcs385fs mkcs385fs.c -g
