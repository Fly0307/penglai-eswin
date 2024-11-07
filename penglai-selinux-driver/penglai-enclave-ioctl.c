#include "penglai-enclave-ioctl.h"
#include "syscall.h"

void printHex(unsigned char *c, int n)
{
	int i;
	for (i = 0; i < n; i+=16) {
		printk("0x%02X, 0x%02X, 0x%02X, 0x%02X,  0x%02X, 0x%02X, 0x%02X, 0x%02X,  0x%02X, 0x%02X, 0x%02X, 0x%02X,  0x%02X, 0x%02X, 0x%02X, 0x%02X, ", 
            c[i], c[i+1], c[i+2], c[i+3], c[i+4], c[i+5], c[i+6], c[i+7], c[i+8], c[i+9], c[i+10], c[i+11], c[i+12], c[i+13], c[i+14], c[i+15]);
	}
}

int penglai_load_and_run_linux(struct file *filep, unsigned long args)
{
    struct penglai_enclave_user_param *enclave_param = (struct penglai_enclave_user_param *)args;
    unsigned long payload_bin_start = (unsigned long)(__va(enclave_param->bin_loadaddr));
    unsigned long payload_dtb_start = (unsigned long)(__va(enclave_param->dtb_loadaddr));
    struct tee_sbi_param_t *tee_sbi_param = kmalloc(sizeof(struct tee_sbi_param_t), GFP_KERNEL);
    int ret;

    ret = load_linux();
    if(ret)
        goto out;

    printk("KERNEL MODULE : hello qy\n");
    printk("KERNEL MODULE : load linux_img: linear va: 0x%lx, and pa: 0x%lx\n", payload_bin_start, (unsigned long)(__pa(payload_bin_start)));
    if(copy_from_user((void*)payload_bin_start, (void*)enclave_param->bin_ptr, enclave_param->bin_size))
	{
		printk("KERNEL MODULE : bin copy from the user is failed\n");
		ret = -EFAULT;
        goto out;
	}

    printk("KERNEL MODULE : load linux dtb: linear va: 0x%lx, and pa: 0x%lx\n", payload_dtb_start, (unsigned long)(__pa(payload_dtb_start)));
    if(copy_from_user((void*)payload_dtb_start, (void*)enclave_param->dtb_ptr, enclave_param->dtb_size))
	{
		printk("KERNEL MODULE : dtb copy from the user is failed\n");
		ret = -EFAULT;
        goto out;
	}

    printk("KERNEL MODULE : load css file\n");
    if(enclave_param->dtb_size < sizeof(enclave_css_t))
    {
        printk("KERNEL MODULE : css file is too small\n");
        ret = -EFAULT;
        goto out;
    }
    if(copy_from_user(&(tee_sbi_param->enclave_css), (void*)enclave_param->css_ptr, sizeof(enclave_css_t)))
    {
        printk("KERNEL MODULE : css copy from the user is failed\n");
		ret = -EFAULT;
        goto out;
    }

    printk("KERNEL MODULE : ecall run sec linux\n");
    tee_sbi_param->bin_loadaddr = enclave_param->bin_loadaddr;
    tee_sbi_param->bin_size = enclave_param->bin_size;
    tee_sbi_param->dtb_loadaddr = enclave_param->dtb_loadaddr;
    tee_sbi_param->dtb_size = enclave_param->dtb_size;

    ret = run_linux(tee_sbi_param);

    printk("KERNEL MODULE : bye\n");

out:
    kfree(tee_sbi_param);
    return ret;
}

int penglai_attest_linux(struct file *filep, unsigned long args)
{
    struct penglai_enclave_ioctl_attest_tee *enclave_param = (struct penglai_enclave_ioctl_attest_tee *)args;
    struct tee_report_t* report = kmalloc(sizeof(struct tee_report_t), GFP_KERNEL);
    int ret;

    printk("KERNEL MODULE : hello qy\n");
    printk("KERNEL MODULE : ecall attest sec linux\n");
    ret = attest_linux(report, enclave_param->nonce);
    if(ret == -2UL){
        printk("KERNEL MODULE : TEE haven't finished boot yet\n");
    }

    enclave_param->report = *report;

    printk("KERNEL MODULE : bye\n");
    kfree(report);
    return ret;
}

long penglai_enclave_ioctl(struct file *filep, unsigned int cmd, unsigned long args)
{
    char ioctl_data[1024];
    int ioc_size, ret;

    ioc_size = _IOC_SIZE(cmd);
    if (ioc_size > sizeof(ioctl_data))
    {
        printk("KERNEL MODULE : ioc_data buff is not enough\n");
        return -EFAULT;
    }

    if (copy_from_user(ioctl_data, (void *)args, ioc_size))
    {
        printk("KERNEL MODULE : copy from the user is failed\n");
        return -EFAULT;
    }

    switch (cmd)
    {
    case PENGLAI_IOC_LOAD_AND_RUN_LINUX:
        ret = penglai_load_and_run_linux(filep, (unsigned long)ioctl_data);
        break;
    case PENGLAI_IOC_ATTEST_LINUX:
        ret = penglai_attest_linux(filep, (unsigned long)ioctl_data);
        break;
    default:
        return -EFAULT;
    }

    if (copy_to_user((void *)args, ioctl_data, ioc_size))
    {
        printk("KERNEL MODULE: ioc_data buff is not enough\n");
        return -EFAULT;
    }
    return ret;
}
