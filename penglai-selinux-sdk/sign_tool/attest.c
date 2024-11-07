#include "attest.h"
#include "util.h"
#include "SM3.h"
#include "SM2_sv.h"
#include <string.h>
#include <stdio.h>

int hash_sec_linux(struct penglai_enclave_user_param *user_param, enclave_css_t *enclave_css)
{
    SM3_STATE hash_ctx;
    unsigned char *hash = enclave_css->enclave_hash;
    unsigned long curr_addr, left_size, counter;
    int hash_granularity = 1 << 20;

    SM3_init(&hash_ctx);

    //hash secure linux image
    printf("[%s] hash sec-linux image load address: %lx\n", __func__, user_param->bin_loadaddr);
    SM3_process(&hash_ctx, (unsigned char*)&user_param->bin_loadaddr, sizeof(unsigned long));

    printf("[%s] Start to hash sec-linux image:\n", __func__);
    curr_addr = user_param->bin_ptr;
    left_size = user_param->bin_size;
    counter = 0;
    while(left_size > hash_granularity){
        SM3_process(&hash_ctx, (unsigned char*)curr_addr, hash_granularity);
        curr_addr += hash_granularity;
        left_size -= hash_granularity;
        counter++;
        printf("[%s] hashed %ld MB, left %ld MB\n", __func__, counter, left_size >> 20);
    }
    SM3_process(&hash_ctx, (unsigned char*)curr_addr, (int)left_size);
    printf("[%s] Finish sec-linux image hash, total %ld B\n", __func__, user_param->bin_size);

    //hash secure dtb
    printf("[%s] hash sec-linux dtb load address: %lx\n", __func__, user_param->dtb_loadaddr);
    SM3_process(&hash_ctx, (unsigned char*)&user_param->dtb_loadaddr, sizeof(unsigned long));

    printf("[%s] Start to hash sec-linux dtb:\n", __func__);
    curr_addr = user_param->dtb_ptr;
    left_size = user_param->dtb_size;
    counter = 0;
    while(left_size > hash_granularity){
        SM3_process(&hash_ctx, (unsigned char*)curr_addr, hash_granularity);
        curr_addr += hash_granularity;
        left_size -= hash_granularity;
        counter++;
        printf("[%s] hashed %ld MB, left %ld MB\n", __func__, counter, left_size >> 20);
    }
    SM3_process(&hash_ctx, (unsigned char*)curr_addr, (int)left_size);
    printf("[%s] Finish sec-linux dtb hash, total %ld B\n", __func__, user_param->dtb_size);

    SM3_done(&hash_ctx, hash);

    return 0;
}

void sign_sec_linux(void* signature_arg, unsigned char *message, int len, unsigned char *prikey_arg)
{
    struct signature_t *signature = (struct signature_t*)signature_arg;
    struct prikey_t *prikey = (struct prikey_t *)prikey_arg;
    
    SM2_Sign(message, len, prikey->dA, (unsigned char *)(signature->r),
        (unsigned char *)(signature->s));
}

int verify_sec_linux(void* signature_arg, unsigned char *message, int len, unsigned char *pubkey_arg)
{
    int ret = 0;
    struct signature_t *signature = (struct signature_t*)signature_arg;
    struct pubkey_t *pubkey = (struct pubkey_t *)pubkey_arg;

    ret = SM2_Verify(message, len, pubkey->xA, pubkey->yA,
        (unsigned char *)(signature->r), (unsigned char *)(signature->s));
    return ret;
}
