#include <stdio.h>
#include <string.h>
#include "util.h"
#include "remote.h"
#include "SM2_sv.h"
#include "SM3.h"

//the device public key
unsigned char dev_pubkey[64] = {
        0x09, 0xF9, 0xDF, 0x31,  0x1E, 0x54, 0x21, 0xA1,  0x50, 0xDD, 0x7D, 0x16,  0x1E, 0x4B, 0xC5, 0xC6,  
        0x72, 0x17, 0x9F, 0xAD,  0x18, 0x33, 0xFC, 0x07,  0x6B, 0xB0, 0x8F, 0xF3,  0x56, 0xF3, 0x50, 0x20,  
        0xCC, 0xEA, 0x49, 0x0C,  0xE2, 0x67, 0x75, 0xA5,  0x2D, 0xC6, 0xEA, 0x71,  0x8C, 0xC1, 0xAA, 0x60,  
        0x0A, 0xED, 0x05, 0xFB,  0xF3, 0x5E, 0x08, 0x4A,  0x66, 0x32, 0xF6, 0x07,  0x2D, 0xA9, 0xAD, 0x13};

int verify_signature(void* signature_arg, unsigned char *message, int len, unsigned char *pubkey_arg)
{
    int ret = 0;
    struct signature_t *signature = (struct signature_t*)signature_arg;
    struct pubkey_t *pubkey = (struct pubkey_t *)pubkey_arg;
    ret = SM2_Verify(message, len, pubkey->xA, pubkey->yA,
        (unsigned char *)(signature->r), (unsigned char *)(signature->s));
    return ret;
}

int main(){
    printf("Hello Remote\n");

    struct tee_report_t report;
    struct custom_message_t* custom_message;
    int pass_auth = 1;

    printf("read digital report from digital_report.txt\n");
    read_file_to_buf("digital_report.txt", (unsigned char *)&report, sizeof(struct tee_report_t), 0);

    if(memcmp(report.sm_pub_key, dev_pubkey, PUBLIC_KEY_SIZE) == 0){
        printf("[Secure Monitor] device's public key is right.\n");
    } else {
        pass_auth = 0;
        printf("[Secure Monitor] device's public key is wrong.\n");
    }

    if(verify_signature(report.signature, (unsigned char*)&report.sig_message,
		sizeof(struct tee_sig_message_t), report.sm_pub_key) == 0){
		printf("[Remote] SUCCESS: Verification of the signature in digital_report succeeded.\n");
	} else {
        pass_auth = 0;
        printf("[Remote] FAILED: Verification of the signature in digital_report failed.\n");
    }

    if(pass_auth){
        printf("[Remote] Authentication passed.\n");
        printf("[Remote] Print custom field:\n");
        custom_message = (struct custom_message_t*)&report.sig_message.custom_field;

        printf("[Remote] magic: %s", custom_message->magic);
        printf("[Remote] ipaddr_info: %s", custom_message->ipaddr_info);
        printf("[Remote] pubkey_footprint: %s", custom_message->pubkey_footprint);
    } else {
        printf("[Remote] Authentication failed.\n");
    }
}
