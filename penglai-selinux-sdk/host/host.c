#include "penglai-enclave.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>

void load_and_run(struct elf_args *enclaveFile, unsigned long bin_loadaddr,
                  struct elf_args *dtbFile, unsigned long dtb_loadaddr, struct elf_args *cssFile)
{
    struct PLenclave *enclave = malloc(sizeof(struct PLenclave));
    PLenclave_init(enclave);

    if (PLenclave_load_and_run(enclave, enclaveFile, bin_loadaddr, dtbFile, dtb_loadaddr, cssFile) < 0)
    {
        printf("host: failed to create enclave\n");
    }

    PLenclave_finalize(enclave);
    free(enclave);
}

void attest(unsigned long nonce)
{
    struct PLenclave *enclave = malloc(sizeof(struct PLenclave));
    struct penglai_enclave_ioctl_attest_tee* attest_param;
    struct tee_report_t* report;
    struct tee_sig_message_t* sig_message;
    PLenclave_init(enclave);

    if (PLenclave_attest(enclave, nonce) < 0)
    {
        printf("host: failed to attest\n");
        goto out;
    }

    printf("***************** ATTEST REPORT *****************\n");
    attest_param = &enclave->attest_param;
    printf("user input nonce: %lx\n", attest_param->nonce);
    report = &attest_param->report;
    printf("sm_signature: \n");
    printHex(report->signature, SIGNATURE_SIZE);
    printf("sm_pub_key: \n");
    printHex(report->sm_pub_key, PUBLIC_KEY_SIZE);
    printf("****************** sig_message ******************\n");
    sig_message = &report->sig_message;
    printf("tee_hash: \n");
    printHex(sig_message->hash, HASH_SIZE);
    printf("tee_custom_field: \n");
    printHex(sig_message->custom_field, TEE_CUSTOM_FIELD_SIZE);
    printf("nonce: \n");
    printHex((unsigned char*)&sig_message->nonce, sizeof(unsigned long));
    printf("****************** sig_message ******************\n");
    printf("***************** ATTEST REPORT *****************\n");

    struct custom_message_t* custom_message = (struct custom_message_t*)&sig_message->custom_field;

    printf("magic: %s", custom_message->magic);
    printf("ipaddr_info: %s", custom_message->ipaddr_info);
    printf("pubkey_footprint: %s", custom_message->pubkey_footprint);

    if(write_data_to_file("digital_report.txt", "wb", (unsigned char *)report, sizeof(struct tee_report_t), 0) != 0){
        printf("failed to write digital report to digital_report.txt\n");
    } else {
        printf("success to write digital report to digital_report.txt\n");
    }

out:
    PLenclave_finalize(enclave);
    free(enclave);
}

static bool cmdline_parse(unsigned int argc, char *argv[], int *mode, const char **path)
{
    assert(mode!=NULL && path != NULL);
    if(argc<2)
    {
        printf("REE_HOST: Lack of parameters.\n");
        return false;
    }
    if(argc == 2 && !strcmp(argv[1], "-help"))
    {
         printf(USAGE_STRING);
         *mode = -1;
         return true;
    }
    
    enum { PAR_REQUIRED, PAR_OPTIONAL, PAR_INVALID };
    typedef struct _param_struct_{
        const char *name;          //options
        char *value;               //keep the path
        int flag;                  //indicate this parameter is required(0), optional(1) or invalid(2)
    }param_struct_t;               //keep the parameter pairs

    param_struct_t params_run[] = {
        {"-image", NULL, PAR_REQUIRED},
        {"-imageaddr", NULL, PAR_REQUIRED},
        {"-dtb", NULL, PAR_REQUIRED},
        {"-dtbaddr", NULL, PAR_REQUIRED},
        {"-cssfile", NULL, PAR_REQUIRED},
        {"-nonce", NULL, PAR_INVALID}};
    param_struct_t params_attest[] = {
        {"-image", NULL, PAR_INVALID},
        {"-imageaddr", NULL, PAR_INVALID},
        {"-dtb", NULL, PAR_INVALID},
        {"-dtbaddr", NULL, PAR_INVALID},
        {"-cssfile", NULL, PAR_INVALID},
        {"-nonce", NULL, PAR_REQUIRED}};

    const char *mode_m[] ={"run", "attest"};
    param_struct_t *params[] = {params_run, params_attest};

	unsigned int tempidx=0;
    for(; tempidx<sizeof(mode_m)/sizeof(mode_m[0]); tempidx++)
    {
        if(!strcmp(mode_m[tempidx], argv[1]))//match
        {
            break;
        }
    }
    unsigned int tempmode = tempidx;
    if(tempmode>=sizeof(mode_m)/sizeof(mode_m[0]))
    {
        printf("Cannot recognize the command \"%s\".\nCommand \"run/attest\" is required.\n", argv[1]);
        return false;
    }

    unsigned int params_count = (unsigned)(sizeof(params_run)/sizeof(params_run[0]));
    for(unsigned int i=2; i<argc; i++)
    {
        unsigned int idx = 0;
        for(; idx<params_count; idx++)
        {
            if(strcmp(argv[i], params[tempmode][idx].name)==0) //match
            {
                if((i<argc-1)&&(strncmp(argv[i+1], "-", 1)))  // assuming pathname doesn't contain "-"
                {
                    if(params[tempmode][idx].value != NULL)
                    {
                        printf("Repeatly specified \"%s\" option.\n", params[tempmode][idx].name);
                        return false;
                    }
                    params[tempmode][idx].value = argv[i+1];
                    i++;
                    break;
                }
                else     //didn't match: 1) no path parameter behind option parameter 2) parameters format error.
                {
                    printf("The File name is not correct for \"%s\" option.\n", params[tempmode][idx].name);
                    return false;
                }
            }
        }
        if(idx == params_count)
        {
            printf("Cannot recognize the option \"%s\".\n", argv[i]);
            return false;
        }
    }

    for(unsigned int i = 0; i < params_count; i++)
    {
        if(params[tempmode][i].flag == PAR_REQUIRED && params[tempmode][i].value == NULL)
        {
            printf("Option \"%s\" is required for the command \"%s\".\n", params[tempmode][i].name, mode_m[tempmode]);
            return false;
        }
        if(params[tempmode][i].flag == PAR_INVALID && params[tempmode][i].value != NULL)
        {
            printf("Option \"%s\" is invalid for the command \"%s\".\n", params[tempmode][i].name, mode_m[tempmode]);
            return false;
        }
    }
    
    for(unsigned int i = 0; i < params_count-1; i++)
    {
        if(params[tempmode][i].value == NULL)
            continue;
        for(unsigned int j=i+1; j < params_count; j++)
        {
            if(params[tempmode][j].value == NULL)
                continue;
            if(strlen(params[tempmode][i].value) == strlen(params[tempmode][j].value) &&
                !strncmp(params[tempmode][i].value, params[tempmode][j].value, strlen(params[tempmode][i].value)))
            {
                printf("Option \"%s\" and option \"%s\" are using the same file path.\n", params[tempmode][i].name, params[tempmode][j].name);
                return false;
            }
        }
    }
    // Set output parameters
    for(unsigned int i = 0; i < params_count; i++)
    {
        path[i] = params[tempmode][i].value;
    }

    *mode = tempmode;
    return true;
}

int main(int argc, char **argv)
{
    printf("Welcome to PENGLAI REE_HOST!\n");
    struct elf_args *enclaveFile = NULL, *dtbFile = NULL, *ccsFile = NULL;

	const char *path[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int res = -1, mode = -1;
	//Parse command line
    if(cmdline_parse(argc, argv, &mode, path) == false)
    {
        printf(USAGE_STRING);
        goto clear_return;
    }
    if(mode == -1) // User only wants to get the help info
    {
        res = 0;
        goto clear_return;
    }
	else if(mode == RUN)
    {
        printf("Load and run tee: \nimage-file: %s, image-addr: %s, dtb-file: %s, dtb-addr: %s, \n", path[IMAGE_FILE], path[IMAGE_LDADDR], path[DTB_FILE], path[DTB_LDADDR]);
        const char *eappfile = path[IMAGE_FILE];
        unsigned long bin_loadaddr = strtol(path[IMAGE_LDADDR], NULL, 16);
        enclaveFile = malloc(sizeof(struct elf_args));
        elf_args_init(enclaveFile, eappfile);
        if (!elf_valid(enclaveFile))
        {
            printf("error when initializing enclaveFile\n");
            goto clear_return;
        }

        const char *dtbfile = path[DTB_FILE];
        unsigned long dtb_loadaddr = strtol(path[DTB_LDADDR], NULL, 16);
        dtbFile = malloc(sizeof(struct elf_args));
        elf_args_init(dtbFile, dtbfile);
        if (!elf_valid(dtbFile))
        {
            printf("error when initializing enclaveFile\n");
            goto clear_return;
        }

        const char *cssfile = path[CCSFILE];
        ccsFile = malloc(sizeof(struct elf_args));
        elf_args_init(ccsFile, cssfile);
        if (!elf_valid(ccsFile))
        {
            printf("error when initializing enclaveFile\n");
            goto clear_return;
        }

        load_and_run(enclaveFile, bin_loadaddr, dtbFile, dtb_loadaddr, ccsFile);
    }
    else if(mode == ATTEST)
    {
        printf("Attest tee: \nnonce: %s\n", path[NONCE]);
        unsigned long nonce = strtol(path[NONCE], NULL, 16);
        attest(nonce);
    }
    printf("Finished.\n");

clear_return:
    if(enclaveFile){
        elf_args_destroy(enclaveFile);
        free(enclaveFile);
    }
    if(dtbFile){
        elf_args_destroy(dtbFile);
        free(dtbFile);
    }
    return 0;
}
