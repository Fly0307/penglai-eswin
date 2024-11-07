#include <linux/mm.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include "penglai-enclave-driver.h"
#include "penglai-enclave-ioctl.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ioctl for secure linux load and run.");
MODULE_AUTHOR("Shangqy");
MODULE_VERSION("penglai_sec_linux_ioctl");

static int enclave_mmap(struct file* f,struct vm_area_struct *vma)
{
	return 0;
}

static const struct file_operations enclave_ops = {
	.owner = THIS_MODULE,
	.mmap = enclave_mmap,
	.unlocked_ioctl = penglai_enclave_ioctl
};

struct miscdevice enclave_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "penglai_sec_linux_dev",
	.fops = &enclave_ops,
	.mode = 0666,
};

int enclave_ioctl_init(void)
{
	int ret;
	printk("penglai_sec_linux_ioctl_init...\n");

	ret=misc_register(&enclave_dev);
	if(ret < 0)
	{
		printk("Enclave_driver: register enclave_dev failed!(ret:%d)\n",
				ret);
		goto deregister_device;
	}

	printk("[Penglai KModule] register penglai_sec_linux_dev succeeded!\n");
	return 0;

deregister_device:
	misc_deregister(&enclave_dev);
	return ret;
}

void enclave_ioctl_exit(void)
{
	printk("penglai_sec_linux_ioctl_exit...\n");

	misc_deregister(&enclave_dev);
	return;
}

module_init(enclave_ioctl_init);
module_exit(enclave_ioctl_exit);
