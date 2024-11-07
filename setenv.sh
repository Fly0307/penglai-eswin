#!/bin/bash
export WORK_DIR=`pwd`
export PATH="/opt/riscv/bin:$PATH"
export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-
# export CROSS_COMPILE=riscv64-unknown-linux-gnu-
# export CROSS_COMPILE=riscv64-unknown-elf-
export RELEASE_TAG=EIC7X-2024.08
export alias openocd="${WORK_DIR}/jtag_tools/openocd_tools/Linux/riscv-openocd-0.11.0-2021.11.1-x86_64-linux-ubuntu14/bin/openocd"
function make_bootchain()
{
    echo "start compile bootchain"
    echo "ddr name:$ddr_name"
    echo "dt name:$dt_name"
    mkdir -p ${WORK_DIR}/$board_name
    rm -rf ${WORK_DIR}/$board_name/uboot-eswin
    # rm -rf ${WORK_DIR}/$board_name/opensbi-eswin
    rm -rf ${WORK_DIR}/$board_name/firmware-eswin
    if [ ! -d $WORK_DIR/source/uboot-eswin ];then
        git clone -b ${RELEASE_TAG} https://github.com/eswincomputing/u-boot.git $WORK_DIR/source/uboot-eswin
    fi
    rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/uboot-eswin ${WORK_DIR}/${board_name}/
    if [ ! -d $WORK_DIR/source/opensbi-eswin ];then
        git clone -b ${RELEASE_TAG} https://github.com/eswincomputing/opensbi.git $WORK_DIR/source/opensbi-eswin
    fi
    rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/opensbi-eswin ${WORK_DIR}/${board_name}/
    rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/firmware-eswin ${WORK_DIR}/${board_name}/
    #uboot
    cd ${WORK_DIR}/${board_name}/uboot-eswin/
    make ${uboot_config}
    sed -i "s#\(CONFIG_DEFAULT_FDT_FILE=\)\"[^\"]*\"#\1\"eswin/${dt_name}.dtb\"#" .config
    make -j$(nproc)
    mkdir -p ${WORK_DIR}/${board_name}/output/
    cp -av u-boot.bin ${WORK_DIR}/${board_name}/output/
    cp -av u-boot.dtb ${WORK_DIR}/${board_name}/output/
    #opensbi
    cd ${WORK_DIR}/${board_name}/opensbi-eswin
    make PLATFORM=eswin/eic770x FW_PAYLOAD=y \
     FW_FDT_PATH=${WORK_DIR}/${board_name}/output/u-boot.dtb \
     FW_PAYLOAD_PATH=${WORK_DIR}/${board_name}/output/u-boot.bin \
     CHIPLET="BR2_CHIPLET_1" \
     CHIPLET_DIE_AVAILABLE="BR2_CHIPLET_1_DIE0_AVAILABLE" \
     MEM_MODE="BR2_MEMMODE_FLAT" \
     PLATFORM_CLUSTER_X_CORE="BR2_CLUSTER_4_CORE" \
    -j $(nproc)
    cp -v build/platform/eswin/eic770x/firmware/fw_payload.bin ../output/fw_payload.bin
    #nsign
    cd ${WORK_DIR}/${board_name}/firmware-eswin
    sed -i "s|out=.*|out=${WORK_DIR}/${board_name}/output/bootloader_${board_name}.bin|" bootchain.config
    secboot_line=`cat -n bootchain.config | grep in= | awk -F " " 'NR==1{print$1}'`
    ddr_line=`cat -n bootchain.config | grep in= | awk -F " " 'NR==2{print$1}'`
    uboot_line=`cat -n bootchain.config | grep in= | awk -F " " 'NR==3{print$1}'`
    sed -i "${secboot_line}s#.*# in=${WORK_DIR}/${board_name}/firmware-eswin/die0_sec_fw.bin#" bootchain.config
    sed -i "${ddr_line}s#.*# in=${WORK_DIR}/${board_name}/firmware-eswin/${ddr_name}#"  bootchain.config
    sed -i "${uboot_line}s#.*# in=${WORK_DIR}/${board_name}/output/fw_payload.bin#"  bootchain.config
    ./nsign bootchain.config
    cd ${WORK_DIR}/${board_name}/output
    ls -l
}

function make_minimal_images()
{
    echo "start compile debian_mkimg"
    desktop_image=$1
    mkdir -p ${WORK_DIR}/$board_name;rm -rf ${WORK_DIR}/$board_name/mkimg-eswin
    if [ -d $WORK_DIR/source/mkimg-eswin ];then
        rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/mkimg-eswin ${WORK_DIR}/${board_name}/
    else
        tar -xf $WORK_DIR/source/mkimg-eswin.tar.gz -C ${WORK_DIR}/${board_name}
    fi
    rm -rf ${WORK_DIR}/${board_name}/penglai-files
    mkdir -p ${WORK_DIR}/${board_name}/penglai-files
    cp -r $WORK_DIR/source/penglai-files/* ${WORK_DIR}/${board_name}/penglai-files
    cd ${WORK_DIR}/$board_name/mkimg-eswin
    chmod +x *.sh
    if [ ! -z ${desktop_image} ];then
        sudo ./mkrootfs.sh ${board_name}
    else
	sudo ./mkrootfs_minimal.sh ${board_name}
    fi
    mkdir -p ${WORK_DIR}/${board_name}/output/
    mv *.ext4 ${WORK_DIR}/${board_name}/output/
    cd ${WORK_DIR}/${board_name}/output
    ls -l
}

function make_secure_boot(){
    mkdir -p ${WORK_DIR}/$board_name;rm -rf ${WORK_DIR}/$board_name/mkimg-eswin
    if [ -d $WORK_DIR/source/mkimg-eswin ];then
        rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/mkimg-eswin ${WORK_DIR}/${board_name}/
    else
        tar -xf $WORK_DIR/source/mkimg-eswin.tar.gz -C ${WORK_DIR}/${board_name}
    fi
    cp -r ${WORK_DIR}/$board_name/output/secure/*.deb ${WORK_DIR}/$board_name/mkimg-eswin
    cp ${WORK_DIR}/$board_name/output/secure/rootfs.cpio.gz ${WORK_DIR}/$board_name/mkimg-eswin
    cd ${WORK_DIR}/$board_name/mkimg-eswin
    chmod +x *.sh
    sudo ./mkboot.sh
    mkdir -p ${WORK_DIR}/${board_name}/output/
    mv -f *.ext4 ${WORK_DIR}/${board_name}/output/
    cd ${WORK_DIR}/${board_name}/output
    ls -l
}

function make_desktop_images()
{
    make_minimal_images "desktop"
}

function make_kernel()
{
    echo "start compile kernel"
    mkdir -p ${WORK_DIR}/$board_name
    # rm -rf ${WORK_DIR}/${board_name}/linux-eswin
    if [ ! -d $WORK_DIR/source/linux-eswin ];then
	git clone --depth=1 -b ${RELEASE_TAG}   https://github.com/eswincomputing/linux-stable.git    $WORK_DIR/source/linux-eswin
    fi
    rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/linux-eswin ${WORK_DIR}/${board_name}/
    cd ${WORK_DIR}/${board_name}/linux-eswin
    export KDEB_PKGVERSION="$(make kernelversion)-$(date "+%Y.%m.%d.%H.%M")+"
    if [ ! -z $1 ];then
        make eic7700_dbg_defconfig
    else
        make eic7700_defconfig
    fi
    #make win2030_defconfig
    make -j$(nproc) bindeb-pkg LOCALVERSION="-eic7x"
    mkdir -p ${WORK_DIR}/${board_name}/output/
    rm -rf ${WORK_DIR}/${board_name}/output/linux*.deb
    mv -v ../*.deb ${WORK_DIR}/${board_name}/output/
    cd ${WORK_DIR}/${board_name}/output
    ls -l
}

function make_secure_kernel()
{
    echo "start compile kernel"
    mkdir -p ${WORK_DIR}/$board_name
    #替换 rootfs.cpio.gz时需要取消注释，删除后重新编译
    rm -rf ${WORK_DIR}/${board_name}/secure-linux-eswin
    if [ ! -d $WORK_DIR/source/secure-linux-eswin ];then
	git clone --depth=1 -b ${RELEASE_TAG}   https://github.com/eswincomputing/linux-stable.git    $WORK_DIR/source/secure-linux-eswin
    fi
    rsync -au --chmod=u=rwX,go=rX  --exclude .git --exclude .hg --exclude .bzr --exclude CVS $WORK_DIR/source/secure-linux-eswin ${WORK_DIR}/${board_name}/
    cd ${WORK_DIR}/${board_name}/secure-linux-eswin
    export KDEB_PKGVERSION="$(make kernelversion)-$(date "+%Y.%m.%d.%H.%M")+"
    if [ ! -z $1 ];then
        make eic7700_dbg_defconfig
    else
        make eic7700_defconfig
    fi
    #make win2030_defconfig
    #拷贝配置文件覆盖(或者手动关闭SMP)
    cp -f ${WORK_DIR}/secure_linuxconfig .config

    # for vmliunx and image
    # make -j$(nproc) vmlinux LOCALVERSION="-eic7x" all

    make -j$(nproc) bindeb-pkg LOCALVERSION="-eic7x"
    rm -rf ${WORK_DIR}/${board_name}/output/secure/
    mkdir -p ${WORK_DIR}/${board_name}/output/secure
    mv -v ../*.deb ${WORK_DIR}/${board_name}/output/secure
    cp  vmlinux ${WORK_DIR}/${board_name}/output/secure
    cp rootfs.cpio.gz ${WORK_DIR}/${board_name}/output/secure

    cd ${WORK_DIR}
    ./scripts/update_penglaifiles.sh
    
    cd ${WORK_DIR}/${board_name}/output/secure
    ls -l
}


function make_debug_kernel()
{
    make_kernel "debug"
}

function make_all()
{
    echo "start make all"
    make_bootchain
    make_kernel
    make_secure_kernel
    make_minimal_images
}

print_comple_method()
{
    echo "you chose $board_name"
    echo "Use the following method to start compiling:"
    echo "    make_bootchain"
    echo "    make_kernel"
    echo "    make_secure_kernel"
    echo "    make_debug_kernel"
    echo "    make_minimal_images"
    echo "    make_desktop_images"
    echo "    make_all:bootchain kernel minimal_images"
}

#menu
echo "board list:"
echo [1] EIDS100A
echo [2] EIDS200A
echo [3] EIDS200B
echo [4] HF106
echo [5] Exit
read -p "please select[1-5]:" -n 1 CHOICE
echo 
while [ 0 -eq 0 ];
do
    if [ "$CHOICE" == "1" ];then
	    export board_name=EIDS100A
	    export ddr_name=ddr_fw.bin
	    export dt_name=eic7700-evb
	    #export uboot_config=eic7700_uboot_defconfig
	    export uboot_config=eic7700_evb_defconfig
	    print_comple_method
	    break
    elif [ "$CHOICE" == "2" ];then
            export board_name=EIDS200A
	    export ddr_name=ddr_fw.bin
	    export dt_name=eic7700-evb
	    #export uboot_config=eic7700_uboot_defconfig
	    export uboot_config=eic7700_evb_defconfig
	    print_comple_method
	    break
    elif [ "$CHOICE" == "3" ];then
	    export board_name=EIDS200B
	    export ddr_name=ddr_fw.bin
	    export dt_name=eic7700-evb-a2
	    #export uboot_config=eic7700_uboot_defconfig
	    export uboot_config=eic7700_evb_defconfig
	    print_comple_method
	    break
    elif [ "$CHOICE" == "4" ];then
            export board_name=HF106
	    export ddr_name=ddr_fw.bin
	    export dt_name=eic7700-hifive-premier-p550
	    #export uboot_config=hifive_premier_550_defconfig
	    export uboot_config=hifive_premier_p550_defconfig
	    print_comple_method
	    break
    elif [ "$CHOICE" == "5" ];then
           echo "Exiting..."
           break
    else
         echo "Invalid option"
	 read -p "please select[1-5]:" -n 1 CHOICE
	 echo 
    fi	 
done
