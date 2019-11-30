obj-m += myfs.o
PWD := $(shell pwd)
KERNELDIR ?= ~/qemu_arm64/linux-4.19/out_aarch64
CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc


default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules ARCH=arm64 CROSS_COMPILE=$(CROSS_COMPILE)
clean:
	@rm -rf *.o *.out *.mod.c *.mod.o *.ko *.order *.symvers .*.cmd .tmp_versions
