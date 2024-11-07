#ifndef _PENGLAI_ENCLAVE_IOCTL
#define _PENGLAI_ENCLAVE_IOCTL
#include "penglai-enclave.h"
#include <linux/uaccess.h>
#include <linux/types.h>
#include <asm/timex.h>

#define PENGLAI_IOC_LOAD_AND_RUN_LINUX \
	_IOR(PENGLAI_ENCLAVE_IOC_MAGIC, 0x00, struct penglai_enclave_user_param)
#define PENGLAI_IOC_ATTEST_LINUX \
	_IOR(PENGLAI_ENCLAVE_IOC_MAGIC, 0x01, struct penglai_enclave_ioctl_attest_tee)

struct penglai_enclave_user_param
{
	unsigned long bin_ptr;
	unsigned long bin_size;
    unsigned long bin_loadaddr;
    unsigned long dtb_ptr;
    unsigned long dtb_size;
    unsigned long dtb_loadaddr;
    unsigned long css_ptr;
    unsigned long css_size;
};

struct penglai_enclave_ioctl_attest_tee
{
	unsigned long nonce;
	struct tee_report_t report;
};

long penglai_enclave_ioctl(struct file* filep, unsigned int cmd, unsigned long args);

#endif
