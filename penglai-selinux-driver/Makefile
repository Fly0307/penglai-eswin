##
## Author: Dong Du
## Description:
## 	The file now assumes the kernel located in ../openeuler-kernel,
##	if you would like to use your own one, please change the path

obj-m += penglai_linux.o
penglai_linux-objs := penglai-enclave-driver.o \
	penglai-enclave.o \
	penglai-enclave-ioctl.o 

all:
	make -C ${WORK_DIR}/HF106/linux-eswin CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv M=$(PWD) modules

clean:
	make -C ${WORK_DIR}/HF106/linux-eswin ARCH=riscv M=$(PWD) clean
