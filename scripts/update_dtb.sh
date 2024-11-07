#!/bin/bash
BOOT_IMG="boot.ext4"
# BOOT_IMG="boot-HF106-20241030-203411.ext4"

cd ${WORK_DIR}/$board_name/output

mkdir -p mnt/boot

sudo umount mnt/boot

echo "mount ${BOOT_IMG} to mnt/boot"
sudo mount ${BOOT_IMG} mnt/boot

echo "copy extlinux.conf to mnt/boot/extlinux/"
sudo mkdir -p mnt/boot/extlinux/
sudo cp ${WORK_DIR}/source/bootfiles_secure/extlinux/extlinux.conf mnt/boot/extlinux/

echo "copy Image to mnt/boot/"
sudo cp ${WORK_DIR}/source/secure-linux-eswin/rootfs.cpio.gz mnt/boot/

echo "copy dtb to mnt/boot/"
sudo cp ${WORK_DIR}/HF106/secure-linux-eswin/arch/riscv/boot/dts/eswin/*.dtb mnt/boot/dtbs/linux-image-6.6.18-eic7x/eswin

echo "umount mnt/boot"
sudo umount mnt/boot
