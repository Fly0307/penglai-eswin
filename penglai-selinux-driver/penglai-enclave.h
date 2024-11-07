#ifndef _PENGLAI_ENCLAVE
#define _PENGLAI_ENCLAVE
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/list.h>
#include <linux/file.h>
#include <asm/sbi.h>
#include <asm/csr.h>
#include "riscv64.h"

#define SBI_EXT_PENGLAI_HOST            0x100100

//define SBI_CALL here
#define SBI_CALL_0(func_id) 	    sbi_ecall(SBI_EXT_PENGLAI_HOST, func_id, 0   , 0   , 0   ,0,0,0)
#define SBI_CALL_1(func_id, arg1) 		sbi_ecall(SBI_EXT_PENGLAI_HOST, func_id, arg1, 0   , 0   ,0,0,0)
#define SBI_CALL_2(func_id, arg1, arg2) 	sbi_ecall(SBI_EXT_PENGLAI_HOST, func_id, arg1, arg2, 0   ,0,0,0)
#define SBI_CALL_3(func_id, arg1, arg2, arg3)	sbi_ecall(SBI_EXT_PENGLAI_HOST, func_id, arg1, arg2, arg3,0,0,0)

#define PENGLAI_ENCLAVE_IOC_MAGIC  0xa4

//SBI CALL NUMBERS
#define SBI_SM_INIT                     100
#define SBI_SM_CREATE_ENCLAVE            99
#define SBI_SM_ATTEST_ENCLAVE            98
#define SBI_SM_RUN_ENCLAVE               97
#define SBI_SM_STOP_ENCLAVE              96
#define SBI_SM_RESUME_ENCLAVE            95
#define SBI_SM_DESTROY_ENCLAVE           94
#define SBI_SM_ALLOC_ENCLAVE_MEM         93
#define SBI_SM_MEMORY_EXTEND             92
#define SBI_SM_FREE_ENCLAVE_MEM          91
#define SBI_SM_RUN_SEC_LINUX             90
#define SBI_SM_DEBUG_PRINT               88
#define SBI_SM_ATTEST_SEC_LINUX          87
#define SBI_SM_LOAD_SEC_LINUX            86


#define PRIVATE_KEY_SIZE       32
#define PUBLIC_KEY_SIZE        64
#define HASH_SIZE              32
#define SIGNATURE_SIZE         64
#define TEE_CUSTOM_FIELD_SIZE  512

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

/****************************************************************************
* Definitions for enclave signature
****************************************************************************/
typedef struct _enclave_css_t {        /* 160 bytes */
    unsigned char enclave_hash[HASH_SIZE];          /* (32) */
    unsigned char signature[SIGNATURE_SIZE];        /* (64) */
    unsigned char user_pub_key[PUBLIC_KEY_SIZE];    /* (64) */
} enclave_css_t;

struct tee_sbi_param_t
{
    unsigned long bin_size;
    unsigned long bin_loadaddr;
    unsigned long dtb_size;
    unsigned long dtb_loadaddr;
    enclave_css_t enclave_css;
};

int load_linux(void);

int run_linux(struct tee_sbi_param_t *tee_sbi_param);

int attest_linux(struct tee_report_t *report, unsigned long nonce);

#endif
