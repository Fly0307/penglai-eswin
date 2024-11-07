#ifndef _ATTEST_H
#define _ATTEST_H

#include "penglai-enclave.h"

/****************************************************************************
* Definitions for enclave signature
****************************************************************************/
typedef struct _enclave_css_t {        /* 160 bytes */
    unsigned char enclave_hash[HASH_SIZE];          /* (32) */
    unsigned char signature[SIGNATURE_SIZE];        /* (64) */
    unsigned char user_pub_key[PUBLIC_KEY_SIZE];    /* (64) */
} enclave_css_t;

int hash_sec_linux(struct penglai_enclave_user_param *user_param, enclave_css_t *enclave_css);

void sign_sec_linux(void* signature, unsigned char *message, int len, unsigned char *prikey_arg);

int verify_sec_linux(void* signature, unsigned char *message, int len, unsigned char *prikey_arg);

#endif
