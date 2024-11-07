#!/bin/bash

# 设置变量
DEB_FILE=$(ls linux-image-*.deb 2>/dev/null | head -n 1)  # 寻找前缀为linux-image-的.deb文件
MOUNT_DIR="./mnt2"
BOOT_IMG="boot.ext4"  # 镜像文件名
# BOOT_SIZE="100M"       # 启动镜像大小
BOOT_SIZE="700M"       # 启动镜像大小
BOOT_UUID="44b7cb94-f58c-4ba6-bfa4-7d2dce09a3a5"


# 创建临时目录
TEMP_DIR=$(mktemp -d)

# 检查临时目录是否创建成功
if [ ! -d "$TEMP_DIR" ]; then
    echo "创建临时目录失败！"
    exit 1
fi

echo "临时目录：$TEMP_DIR"

# 解压 .deb 文件
echo "解压 .deb 文件..."
dpkg-deb -x "$DEB_FILE" "$TEMP_DIR"

# 检查解压是否成功
if [ $? -ne 0 ]; then
    echo "解压 .deb 文件失败！"
    exit 1
fi

# 创建启动镜像
echo "创建启动镜像：$BOOT_IMG"
truncate -s "$BOOT_SIZE" "$BOOT_IMG"  # 使用 truncate 创建指定大小的镜像文件
mkfs.ext4 -F -O ^metadata_csum "$BOOT_IMG"                  # 格式化为 ext4 文件系统
tune2fs -U $BOOT_UUID $BOOT_IMG

# 挂载启动镜像
sudo mkdir -p "$MOUNT_DIR"
sudo mount -o loop "$BOOT_IMG" "$MOUNT_DIR"

# 拷贝 boot 文件夹的内容到启动镜像
echo "拷贝 boot 目录内容到启动镜像..."
ls -l -h "$TEMP_DIR/boot"
sudo cp -r "$TEMP_DIR/boot/"* "$MOUNT_DIR/"
sudo cp ./rootfs.cpio.gz "$MOUNT_DIR/"
ls -l -h "$MOUNT_DIR/"



# 卸载镜像
echo "卸载启动镜像..."
sudo umount "$MOUNT_DIR"

# 清理临时文件
rm -rf "$TEMP_DIR"
rmdir "$MOUNT_DIR"
rm -f *.deb

echo "启动镜像 $BOOT_IMG 创建成功！"
