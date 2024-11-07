#!/usr/bin/env bash

BOOT_SIZE=500M
#BOOT_SIZE=100M
BOOT_IMG=""
ROOT_SIZE=7G
ROOT_IMG=""
CHROOT_TARGET=rootfs
BOOT_UUID="44b7cb94-f58c-4ba6-bfa4-7d2dce09a3a5"
ROOT_UUID="80a5a8e9-c744-491a-93c1-4f4194fd690a"
BOARD=$1

if [ -f ../output/linux-image-*-dbg*.deb ];then
    echo debug kernel
    DEBUG_KERNEL_DEB="linux-image-6.6.18-eic7x-dbg"
    ROOT_SIZE=9G
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
    PACKAGE_LIST="ca-certificates cloud-guest-utils neofetch network-manager debian-archive-keyring u-boot-menu sudo initramfs-tools locales bluez blueman mpv chromium systemd-timesyncd"
    mmdebstrap --architectures=riscv64 \
        --include="$PACKAGE_LIST" \
        sid $1 \
        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-gles/ rockos-gles main" \
        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-media-new/ rockos-media-new main" \
        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-kernels/ rockos-kernels main" \
        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-addons/ rockos-addons main" \
        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/rockos-base/ sid main contrib non-free non-free-firmware"

#        "deb [trusted=yes] http://10.9.126.103:83/dev/rockos-kernels/ rockos-kernels main" \
#        "deb [trusted=yes] http://10.9.126.103:83/dev/rockos-addons/ rockos-addons main" \
#        "deb [trusted=yes] http://10.9.126.103:83/rockos-base/ sid main contrib non-free non-free-firmware"

#        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-pg/ rockos-pg main" \
#        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/dev/rockos-addons/ rockos-addons main" \
#        "deb [trusted=yes] https://rockos:HJcz78SxbyMGw42Ny8rM@mirror.iscas.ac.cn/rockos/rockos-base/ sid main contrib non-free non-free-firmware"
}

make_rootfs()
{
    make_rootfs_tarball $CHROOT_TARGET
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/boot/)" ]; then
        mkdir "$CHROOT_TARGET"/mnt/boot
        mv -v "$CHROOT_TARGET"/boot/* "$CHROOT_TARGET"/mnt/boot/
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
    mount -t tmpfs tmpfs "$CHROOT_TARGET"/var/cache/apt/archives/

    # move boot contents back to /boot
    if [ ! -z "$(ls -A "$CHROOT_TARGET"/mnt/boot/)" ]; then
        mv -v "$CHROOT_TARGET"/mnt/boot/* "$CHROOT_TARGET"/boot/
        rmdir "$CHROOT_TARGET"/mnt/boot
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
        chroot "$CHROOT_TARGET" sh -c 'dpkg -i /root/*.deb'
        chroot "$CHROOT_TARGET" sh -c 'apt install -f'
        chroot "$CHROOT_TARGET" sh -c 'apt update && apt install -y linux-image-6.6.18-eic7x ${DEBUG_KERNEL_DEB} linux-headers-6.6.18-eic7x'
    fi

    # Add update-u-boot config
    cat > $CHROOT_TARGET/etc/default/u-boot << EOF
U_BOOT_PROMPT="2"
U_BOOT_MENU_LABEL="RockOS GNU/Linux"
U_BOOT_PARAMETERS="console=tty0 console=ttyS0,115200 root=UUID=${ROOT_UUID} rootwait rw earlycon selinux=0 LANG=en_US.UTF-8 audit=0"
U_BOOT_ROOT="root=UUID=${ROOT_UUID}"
U_BOOT_FDT_DIR="/dtbs/linux-image-"
EOF

    # Update extlinux config
    chroot "$CHROOT_TARGET" sh -c "u-boot-update"
}

after_mkrootfs()
{

    # Set locale to en_US.UTF-8 UTF-8
    chroot "$CHROOT_TARGET" sh -c "echo 'locales locales/default_environment_locale select en_US.UTF-8' | debconf-set-selections"
    chroot "$CHROOT_TARGET" sh -c "echo 'locales locales/locales_to_be_generated multiselect en_US.UTF-8 UTF-8' | debconf-set-selections"
    chroot "$CHROOT_TARGET" sh -c "rm /etc/locale.gen"
    chroot "$CHROOT_TARGET" sh -c "dpkg-reconfigure --frontend noninteractive locales"

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
    chroot "$CHROOT_TARGET" sh -c "useradd -m -s /bin/bash -G adm,cdrom,floppy,sudo,input,audio,dip,video,plugdev,netdev,bluetooth,lp eswin"
    chroot "$CHROOT_TARGET" sh -c "echo 'eswin:eswin' | chpasswd"

    # Copy kvm
    #cp -v kvm/* $CHROOT_TARGET/home/debian/
    chroot "$CHROOT_TARGET" sh -c 'chown eswin:eswin /home/eswin/*'

    # Change hostname
    chroot $CHROOT_TARGET /bin/bash << EOF
echo rockos-eswin > /etc/hostname
echo "127.0.1.1 rockos-eswin" >> /etc/hosts
exit
EOF

    # xfce desktop
    chroot $CHROOT_TARGET /bin/bash << EOF
export DEBIAN_FRONTEND=noninteractive
apt install -y task-xfce-desktop
apt install -y qemu-system
apt install -y bash-completion
apt install -y firmware-amd-graphics
apt install -y mesa-vulkan-drivers mesa-va-drivers mesa-vdpau-drivers
apt install -y ssh
apt install -y glmark2-es2 mesa-utils vulkan-tools
apt install -y supertuxkart
exit
EOF

    # fix pulseaudio
    chroot $CHROOT_TARGET /bin/bash << EOF
echo "default-sample-format = s32le" >> /etc/pulse/daemon.conf
echo "default-sample-rate = 48000" >> /etc/pulse/daemon.conf
echo "alternate-sample-rate = 96000" >> /etc/pulse/daemon.conf
sed -i 's/load-module module-udev-detect/load-module module-udev-detect tsched=1 tsched_buffer_size=8192/' /etc/pulse/system.pa
sed -i 's/load-module module-udev-detect/load-module module-udev-detect tsched=1 tsched_buffer_size=8192/' /etc/pulse/default.pa
exit
EOF

    # copy kernel module
#    mkdir -p $CHROOT_TARGET/lib/modules/5.17.0-rc7-win2030/extra/
#    cp -vf ko/*.ko $CHROOT_TARGET/lib/modules/5.17.0-rc7-win2030/extra/
#    cp -vf ko/vdec_driver_load.sh $CHROOT_TARGET/sbin/
#    chroot $CHROOT_TARGET /bin/bash -c 'depmod 5.17.0-rc7-win2030 -a'

    # media desktop
    chroot $CHROOT_TARGET /bin/bash << EOF
export DEBIAN_FRONTEND=noninteractive
apt install -y mpv ffmpeg
exit
EOF

    # gles/media desktop
    chroot $CHROOT_TARGET /bin/bash << EOF
export DEBIAN_FRONTEND=noninteractive
apt install -y rockos-gles-addons
apt install -y libqt5gui5-gles python3-opencv
exit
EOF

     cat << EOF > "$CHROOT_TARGET"/usr/share/X11/xorg.conf.d/10-pvr.conf
Section "Device"
	Identifier "Card1"
	Driver "modesetting"
	Option "kmsdev" "/dev/dri/card0"
	Option "UseGammaLUT" "false"
#	Option "SWcursor" "true"
EndSection

Section "OutputClass"
	Identifier "es_drm_display"
	MatchDriver "es_drm"
#	Option	"PrimaryGPU"	"true"
EndSection
EOF

    cat << EOF > "$CHROOT_TARGET"/etc/powervr.ini
[supertuxkart]
DisableFBCDC=1
EOF

# for lightdm
    cp -vf addons/80-workaround-lightdm-X-on-drm-hotplug.rules "$CHROOT_TARGET"/etc/udev/rules.d/
    cp -vf addons/kill-lightdm-X "$CHROOT_TARGET"/usr/libexec/
    chroot $CHROOT_TARGET /bin/bash -c 'chmod a+x /usr/libexec/kill-lightdm-X'
    #mkdir -p "$CHROOT_TARGET"/etc/systemd/system/lightdm.service.d/
    #cp -vf addons/override.conf "$CHROOT_TARGET"/etc/systemd/system/lightdm.service.d/

    # for wifi
    chroot $CHROOT_TARGET /bin/bash << EOF
export DEBIAN_FRONTEND=noninteractive
apt install -y wpasupplicant
EOF
    cp -vf addons/10-wifi.conf "$CHROOT_TARGET"/etc/NetworkManager/conf.d/
    mkdir -p "$CHROOT_TARGET"/lib/firmware/eic7x
    cp -vf firmware/* "$CHROOT_TARGET"/lib/firmware/eic7x

    # for auto load module
    cat addons/modules.conf >> "$CHROOT_TARGET"/etc/modules-load.d/modules.conf
    cat "$CHROOT_TARGET"/etc/modules-load.d/modules.conf

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
