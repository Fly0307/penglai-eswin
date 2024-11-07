#!/bin/bash
PENGLAI_FLIES="${WORK_DIR}/source/penglai-files"

cd ${WORK_DIR}/penglai-selinux-driver/
#update penglai_linux.ko
make 
echo "copy penglai_linux.ko"
cp penglai_linux.ko ${PENGLAI_FLIES}

cd ${WORK_DIR}/penglai-selinux-sdk
cp host/host ${PENGLAI_FLIES}

#update dtb
echo "copy eic7700-hifive-premier-p550.dtb"
cp -f ${WORK_DIR}/HF106/secure-linux-eswin/debian/linux-image/boot/dtbs/linux-image-6.6.18-eic7x/eswin/eic7700-hifive-premier-p550.dtb ${PENGLAI_FLIES}

#update secure image
cd ${PENGLAI_FLIES}
echo "copy rootfs.cpio.gz"
cp ${WORK_DIR}/source/secure-linux-eswin/rootfs.cpio.gz .
echo "copy vmlinuz-6.6.18-eic7x"
cp ${WORK_DIR}/HF106/secure-linux-eswin/debian/linux-image/boot/vmlinuz-6.6.18-eic7x .
rm -f secure_linux.img
cp vmlinuz-6.6.18-eic7x vmlinuz-6.6.18-eic7x.gz
gunzip -c vmlinuz-6.6.18-eic7x.gz > secure_img.img
rm -f vmlinuz-6.6.18-eic7x.gz

echo "make run.sh"
rm -f run.sh
echo "#!/bin/bash" > run.sh
echo "sudo /usr/sbin/insmod penglai_linux.ko" >> run.sh
echo "./host run -image secure_img.img -imageaddr 0xc0000000 -dtb eic7700-hifive-premier-p550.dtb -dtbaddr 0x186000000 -cssfile test.css" >> run.sh
echo '# 添加持续输出的提示' >> run.sh
echo 'while true; do' >> run.sh
echo '    echo "启动执行完毕。如果您需要，请按 Ctrl+C 退出。"' >> run.sh
echo '    sleep 3  # 暂停5秒再输出，避免不会刷屏' >> run.sh
echo 'done' >> run.sh

chmod +x run.sh

cd ${PENGLAI_FLIES}
ls -l -h