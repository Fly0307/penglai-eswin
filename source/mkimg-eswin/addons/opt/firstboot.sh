#!/bin/sh

# whiptail --infobox "Resizing filesystem..." 20 60

# use all emmc free space for rootfs
#parted -s /dev/mmcblk0 "resizepart 4 -0"
growpart /dev/mmcblk0 6

# refresh filesystem usable size
resize2fs /dev/mmcblk0p4
resize2fs /dev/mmcblk0p6

# whiptail --infobox "Enable SWAP partition..." 20 60

# formatand enable swap partition
mkswap /dev/mmcblk0p4
swapon -a

# mount non-rootfs
mount /dev/mmcblk0p4 /boot

# regenerate fstab
genfstab -t PARTUUID / > /etc/fstab
update-initramfs -u

# whiptail --infobox "Generating SSH Host keys..." 20 60

# regenerate openssh host keys
dpkg-reconfigure openssh-server

# set hosts
echo "127.0.1.1 $(hostname)" >> /etc/hosts

# whiptail --infobox "Firstboot Done. Rebooting in 3 seconds..." 20 60
# sleep 3
# reboot
