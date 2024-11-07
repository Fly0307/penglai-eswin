#ifndef _PL_ENCLAVE
#define _PL_ENCLAVE
#include "elf.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <pthread.h>
#include <string.h>

#define PENGLAI_ENCLAVE_DEV_PATH "/dev/penglai_sec_linux_dev"

#define PENGLAI_ENCLAVE_IOC_MAGIC 0xa4

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

#define PRIVATE_KEY_SIZE       32
#define PUBLIC_KEY_SIZE        64
#define HASH_SIZE              32
#define SIGNATURE_SIZE         64
#define TEE_CUSTOM_FIELD_SIZE  512

struct custom_message_t
{
    unsigned char magic[16];
    unsigned char ipaddr_info[128];
    unsigned char pubkey_footprint[128];
};

struct tee_sig_message_t
{
    unsigned char hash[HASH_SIZE];
    unsigned char custom_field[TEE_CUSTOM_FIELD_SIZE];
    unsigned long nonce;
};

struct tee_report_t
{
    struct tee_sig_message_t sig_message;
    unsigned char signature[SIGNATURE_SIZE];
    unsigned char sm_pub_key[PUBLIC_KEY_SIZE];
};

struct prikey_t
{
  unsigned char dA[PRIVATE_KEY_SIZE];
};

struct pubkey_t
{
  unsigned char xA[PUBLIC_KEY_SIZE/2];
  unsigned char yA[PUBLIC_KEY_SIZE/2];
};

struct signature_t
{
    unsigned char r[PUBLIC_KEY_SIZE/2];
    unsigned char s[PUBLIC_KEY_SIZE/2];
};

struct penglai_enclave_ioctl_attest_tee
{
    unsigned long nonce;
    struct tee_report_t report;
};

struct PLenclave
{
    struct elf_args *bin_file;
    struct elf_args *dtb_file;
    struct elf_args *css_file;
    int fd;
    struct penglai_enclave_user_param user_param;
    struct penglai_enclave_ioctl_attest_tee attest_param;
};

void PLenclave_init(struct PLenclave *PLenclave);
void PLenclave_finalize(struct PLenclave *PLenclave);
int PLenclave_load_and_run(struct PLenclave *PLenclave,
                           struct elf_args *u_bin_file, unsigned long bin_loadaddr,
                           struct elf_args *u_dtb_file, unsigned long dtb_loadaddr, struct elf_args *cssFile);
int PLenclave_attest(struct PLenclave *PLenclave, unsigned long nonce);

#endif
