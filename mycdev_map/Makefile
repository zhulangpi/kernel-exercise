obj-m += mycdev_map.o
PWD := $(shell pwd)
KERNELDIR ?= /home/zlp/qemu_arm64/linux-5.0/out_aarch64
CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc


default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules ARCH=arm64 CROSS_COMPILE=$(CROSS_COMPILE)
	$(CC) read_test.c -o read_test.out
	$(CC) write_test.c -o write_test.out
clean:
	@rm -rf *.o *.out *.mod.c *.mod.o *.ko *.order *.symvers .*.cmd .tmp_versions
