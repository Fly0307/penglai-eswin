#!/usr/bin/env bash

# BOOT_SIZE=500M
BOOT_SIZE=100M
BOOT_IMG=""
ROOT_SIZE=1500M
ROOT_IMG=""
CHROOT_TARGET=rootfs
BOOT_UUID="44b7cb94-f58c-4ba6-bfa4-7d2dce09a3a5"
ROOT_UUID="80a5a8e9-c744-491a-93c1-4f4194fd690a"
BOARD=$1
HOMEFILE="../penglai-files"

if [ -f ../output/linux-image-*-dbg*.deb ];then
    echo debug kernel
    DEBUG_KERNEL_DEB="linux-image-6.6.18-eic7x-dbg"
    ROOT_SIZE=3G
fi

TIMESTAMP=$(date +%Y%m%d-%H%M%S)

make_imagefile()
{
    BOOT_IMG="boot-$BOARD-$TIMESTAMP.ext4"
    truncate -s "$BOOT_SIZE" "$BOOT_IMG"
    ROOT_IMG="root-$BOARD-$TIMESTAMP.ext4"
    truncate -s "$ROOT_SIZE" "$ROOT_IMG"

    # Format partitions
    mkfs.ext4 -F -O ^metadata_csum "$BOOT_IMG"
    mkfs.ext4 -F -O ^metadata_csum "$ROOT_IMG"

    # UUID
    tune2fs -U $BOOT_UUID $BOOT_IMG
    tune2fs -U $ROOT_UUID $ROOT_IMG
}

pre_mkrootfs()
{
    # Mount loop device
    mkdir "$CHROOT_TARGET"
    mount "$ROOT_IMG" "$CHROOT_TARGET"
}

unmount_image()
{
	echo "Finished and cleaning..."
	if mount | grep "$CHROOT_TARGET" > /dev/null; then
		umount -l "$CHROOT_TARGET"
	fi
	if [ "$(ls -A $CHROOT_TARGET)" ]; then
		echo "folder not empty! umount may fail!"
		exit 2
	else
		echo "Deleting chroot temp folder..."
		if [ -d "$CHROOT_TARGET" ]; then
			rmdir -v "$CHROOT_TARGET"
		fi
		echo "Done."
	fi
}

make_rootfs_tarball()
{
    # use $1
#    PACKAGE_LIST="$KEYRINGS $GPU_DRIVER $BASE_TOOLS $GRAPHIC_TOOLS $XFCE_DESKTOP $BENCHMARK_TOOLS $FONTS $INCLUDE_APPS $EXTRA_TOOLS $LIBREOFFICE"
    PACKAGE_LIST="ca-certificates cloud-guest-utils network-manager rockos-keyring u-boot-menu sudo initramfs-tools openssh-client"
    mmdebstrap --architectures=riscv64 \
        --include="$PACKAGE_LIST" \
        --skip check/empty \
        sid $1 \
        "deb [trusted=yes]  https://mirror.iscas.ac.cn/rockos/20240830/rockos-gles/ rockos-gles main" \
        "deb [trusted=yes]  https://mirror.iscas.ac.cn/rockos/20240830/rockos-media-new/ rockos-media-new main" \
        "deb [trusted=yes]  https://mirror.iscas.ac.cn/rockos/20240830/rockos-kernels/ rockos-kernels main" \
        "deb [trusted=yes]  https://mirror.iscas.ac.cn/rockos/20240830/rockos-addons/ rockos-addons main" \
        "deb [trusted=yes]  https://mirror.iscas.ac.cn/rockos/20240830/rockos-base/ sid main contrib non-free non-free-firmware"

        # "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-gles/ rockos-gles main" \
        # "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-media-new/ rockos-media-new main" \
        # "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-kernels/ rockos-kernels main" \
        # "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-addons/ rockos-addons main" \
        # "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/rockos-base/ sid main contrib non-free non-free-firmware"
}

make_rootfs()
{
    mkdir -p $CHROOT_TARGET/tmp
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/tmp
    mkdir -p $CHROOT_TARGET/var/tmp
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/var/tmp
    mkdir -p $CHROOT_TARGET/var/cache
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/var/cache
    make_rootfs_tarball $CHROOT_TARGET
    umount "$CHROOT_TARGET"/var/cache
    umount "$CHROOT_TARGET"/var/tmp
    umount "$CHROOT_TARGET"/tmp
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/boot/)" ]; then
        mkdir "$CHROOT_TARGET"/mnt/boot
        mv -v "$CHROOT_TARGET"/boot/* "$CHROOT_TARGET"/mnt/boot/
    fi
    # Copy home files
    mkdir -p "$CHROOT_TARGET"/home/eswin/penglai
    cp -r $HOMEFILE/* "$CHROOT_TARGET"/home/eswin/penglai
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/home/eswin/)" ]; then
        mkdir "$CHROOT_TARGET"/mnt/home
        mv -v "$CHROOT_TARGET"/home/* "$CHROOT_TARGET"/mnt/home/
    fi

    # Mount chroot path
    mount "$BOOT_IMG" "$CHROOT_TARGET"/boot
    mount -t proc /proc "$CHROOT_TARGET"/proc
    mount -B /sys "$CHROOT_TARGET"/sys
    mount -B /run "$CHROOT_TARGET"/run
    mount -B /dev "$CHROOT_TARGET"/dev
    mount -B /dev/pts "$CHROOT_TARGET"/dev/pts
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/tmp
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/var/tmp
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/var/cache

    echo ""$CHROOT_TARGET"/boot files:"
    ls "$CHROOT_TARGET"/boot

    echo ""$CHROOT_TARGET"/mnt/boot"
    ls "$CHROOT_TARGET"/mnt/boot

    # move boot contents back to /boot
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/mnt/boot/)" ]; then
        mv -v "$CHROOT_TARGET"/mnt/boot/* "$CHROOT_TARGET"/boot/
        rmdir "$CHROOT_TARGET"/mnt/boot
    fi
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/mnt/home/)" ]; then
        mv -v "$CHROOT_TARGET"/mnt/home/* "$CHROOT_TARGET"/home/
        rmdir "$CHROOT_TARGET"/mnt/home
    fi
    # apt update
    chroot "$CHROOT_TARGET" sh -c "apt update"
}

make_bootable()
{
    # Install kernel
    deb_package=$(ls -l ../output/ | grep "riscv64.deb")
    if [ "$deb_package" != "" ];then
        cp -v ../output/*.deb $CHROOT_TARGET/root/
        echo "before "$CHROOT_TARGET"/boot files:"
        ls "$CHROOT_TARGET"/boot
        chroot "$CHROOT_TARGET" sh -c 'dpkg -i /root/*.deb'
        echo "after "$CHROOT_TARGET"/boot files:"
        ls "$CHROOT_TARGET"/boot
        chroot "$CHROOT_TARGET" sh -c 'apt install -f'
        chroot "$CHROOT_TARGET" sh -c 'apt update && apt install -y linux-image-6.6.18-eic7x $DEBUG_KERNEL_DEB linux-headers-6.6.18-eic7x'
        echo "after "$CHROOT_TARGET"/boot files:"
        ls "$CHROOT_TARGET"/boot
    fi
    #chroot "$CHROOT_TARGET" sh -c 'apt update && apt install -y linux-image-6.6.36-win2030 eic770x-firmware'

    # Add update-u-boot config
    cat > $CHROOT_TARGET/etc/default/u-boot << EOF
U_BOOT_PROMPT="2"
U_BOOT_MENU_LABEL="RockOS GNU/Linux"
U_BOOT_PARAMETERS="console=tty0 console=ttyS0,115200 root=UUID=${ROOT_UUID} rootfstype=ext4 rootwait rw earlycon selinux=0 LANG=en_US.UTF-8 audit=0"
U_BOOT_ROOT="root=UUID=${ROOT_UUID}"
U_BOOT_FDT_DIR="/dtbs/linux-image-"
EOF

    # Update extlinux config
    chroot "$CHROOT_TARGET" sh -c "u-boot-update"
}

after_mkrootfs()
{
    # Set default timezone to Asia/Shanghai
    chroot "$CHROOT_TARGET" sh -c "ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime"
    echo "Asia/Shanghai" > $CHROOT_TARGET/etc/timezone

    # Set up fstab
    chroot $CHROOT_TARGET /bin/bash << EOF
echo 'UUID=${ROOT_UUID} /   auto    defaults    1 1' >> /etc/fstab
echo 'UUID=${BOOT_UUID} /boot   auto    defaults    0 0' >> /etc/fstab

exit
EOF

    # Add user
    chroot "$CHROOT_TARGET" sh -c "useradd -m -s /bin/bash -G adm,cdrom,floppy,sudo,input,audio,dip,video,plugdev,netdev eswin"
    chroot "$CHROOT_TARGET" sh -c "echo 'eswin:eswin' | chpasswd"

    # Change hostname
    chroot $CHROOT_TARGET /bin/bash << EOF
echo rockos-eswin > /etc/hostname
echo "127.0.1.1 rockos-eswin" >> /etc/hosts
exit
EOF
    

    #firmware
    mkdir -p "$CHROOT_TARGET"/lib/firmware/eic7x
    cp -vf firmware/* "$CHROOT_TARGET"/lib/firmware/eic7x
    # enable firstboot
    cp -vf addons/opt/firstboot.sh "$CHROOT_TARGET"/opt
    cp -vf addons/opt/firstboot.service "$CHROOT_TARGET"/etc/systemd/system/firstboot.service
    chroot "$CHROOT_TARGET" sh -c "systemctl enable firstboot"

    # add udevs rules
    cp -vf rules/* "$CHROOT_TARGET"/etc/udev/rules.d/ 
    sed -i '/SUBSYSTEMS=="platform", ENV{SOUND_FORM_FACTOR}="internal".*/d' "$CHROOT_TARGET"/usr/lib/udev/rules.d/78-sound-card.rules
}

make_imagefile
pre_mkrootfs
make_rootfs
make_bootable
after_mkrootfs
unmount_image
