#include <stdio.h>
#include <stdlib.h>
#include "attest.h"
#include "riscv64.h"
#include "util.h"
#include "parse_key_file.h"

typedef enum _file_path_t
{
    IMAGE_FILE = 0,
    IMAGE_LDADDR = 1,
    DTB_FILE,
    DTB_LDADDR,
    KEY,
    OUTPUT,
    SIG,
    UNSIGNED,
    CCSFILE,
    DUMPFILE
} file_path_t;

const char *path[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/*
   load images to memory and calculate the measurement,
   which will be saved in enclave_css 
 */
int load_secure_linux(enclave_css_t *enclave_css)
{
    int ret = 0;
    struct elf_args *enclaveFile = NULL, *dtbFile = NULL;
    struct penglai_enclave_user_param* user_param;

    const char *binfile = path[IMAGE_FILE];
    unsigned long bin_loadaddr = strtol(path[IMAGE_LDADDR], NULL, 16);
    enclaveFile = malloc(sizeof(struct elf_args));
    elf_args_init(enclaveFile, binfile);
    if (!elf_valid(enclaveFile))
    {
        printf("error when initializing enclaveFile\n");
        goto out;
    }

    const char *dtbfile = path[DTB_FILE];
    unsigned long dtb_loadaddr = strtol(path[DTB_LDADDR], NULL, 16);
    dtbFile = malloc(sizeof(struct elf_args));
    elf_args_init(dtbFile, dtbfile);
    if (!elf_valid(dtbFile))
    {
        printf("error when initializing enclaveFile\n");
        goto out;
    }

    user_param = (struct penglai_enclave_user_param *)malloc(sizeof(struct penglai_enclave_user_param));
    user_param->bin_ptr = (unsigned long)enclaveFile->ptr;
    user_param->bin_size = enclaveFile->size;
    user_param->bin_loadaddr = bin_loadaddr;
    user_param->dtb_ptr = (unsigned long)dtbFile->ptr;
    user_param->dtb_size = dtbFile->size;
    user_param->dtb_loadaddr = dtb_loadaddr;

    ret = hash_sec_linux(user_param, enclave_css);

    free(user_param);
out:
    if(enclaveFile){
        elf_args_destroy(enclaveFile);
        free(enclaveFile);
    }
    if(dtbFile){
        elf_args_destroy(dtbFile);
        free(dtbFile);
    }
    return ret;
}

// int update_metadata(const char *eappfile, const enclave_css_t *enclave_css, uint64_t meta_offset)
// {
//     if(eappfile == NULL || enclave_css == NULL || meta_offset < 0){
// 		printf("ERROR: invalid params\n");
// 		return -1;
// 	};
//     return write_data_to_file(eappfile, "rb+", (unsigned char *)enclave_css, sizeof(enclave_css_t), meta_offset);
// }

// int read_metadata(const char *eappfile, enclave_css_t *enclave_css, uint64_t meta_offset)
// {
//     if(eappfile == NULL || enclave_css == NULL || meta_offset < 0){
// 		printf("ERROR: invalid params\n");
// 		return -1;
// 	};
//     return read_file_to_buf(eappfile, (unsigned char *)enclave_css, sizeof(enclave_css_t), meta_offset);
// }

// bool dump_enclave_metadata(const char *eappfile, const char *dumpfile)
// {
//     enclave_css_t enclave_css;
//     unsigned long meta_offset;
//     int ret = 0;

//     ret = load_secure_linux(eappfile, &enclave_css, &meta_offset);
//     if(ret != 0){
//         return false;
//     }

//     memset(&enclave_css, 0, sizeof(enclave_css_t));
//     ret = read_metadata(eappfile, &enclave_css, meta_offset);
//     if(ret != 0){
//         return false;
//     }

//     ret = write_data_to_file(dumpfile, "wb", (unsigned char *)&enclave_css, sizeof(enclave_css_t), 0);
//     if(ret != 0){
//         return false;
//     }
//     return true;
// }

static bool cmdline_parse(unsigned int argc, char *argv[], int *mode, const char **path)
{
    assert(mode!=NULL && path != NULL);
    if(argc<2)
    {
        printf("SIGN_TOOL: Lack of parameters.\n");
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

    param_struct_t params_sign[] = {
        {"-image", NULL, PAR_REQUIRED},
        {"-imageaddr", NULL, PAR_REQUIRED},
        {"-dtb", NULL, PAR_REQUIRED},
        {"-dtbaddr", NULL, PAR_REQUIRED},
        {"-key", NULL, PAR_REQUIRED},
        {"-out", NULL, PAR_REQUIRED},
        {"-sig", NULL, PAR_INVALID},
        {"-unsigned", NULL, PAR_INVALID},
        {"-ccsfile", NULL, PAR_INVALID},
        {"-dumpfile", NULL, PAR_OPTIONAL}};
    param_struct_t params_gendata[] = {
        {"-image", NULL, PAR_REQUIRED},
        {"-imageaddr", NULL, PAR_REQUIRED},
        {"-dtb", NULL, PAR_REQUIRED},
        {"-dtbaddr", NULL, PAR_REQUIRED},
        {"-key", NULL, PAR_INVALID},
        {"-out", NULL, PAR_REQUIRED},
        {"-sig", NULL, PAR_INVALID},
        {"-unsigned", NULL, PAR_INVALID},
        {"-ccsfile", NULL, PAR_INVALID},
        {"-dumpfile", NULL, PAR_INVALID}};
    param_struct_t params_catsig[] = {
        {"-image", NULL, PAR_REQUIRED},
        {"-imageaddr", NULL, PAR_REQUIRED},
        {"-dtb", NULL, PAR_REQUIRED},
        {"-dtbaddr", NULL, PAR_REQUIRED},
        {"-key", NULL, PAR_REQUIRED},
        {"-out", NULL, PAR_REQUIRED},
        {"-sig", NULL, PAR_REQUIRED},
        {"-unsigned", NULL, PAR_REQUIRED},
        {"-ccsfile", NULL, PAR_INVALID},
        {"-dumpfile", NULL, PAR_OPTIONAL}};
    param_struct_t params_dump[] = {
        {"-image", NULL, PAR_INVALID},
        {"-imageaddr", NULL, PAR_INVALID},
        {"-dtb", NULL, PAR_INVALID},
        {"-dtbaddr", NULL, PAR_INVALID},
        {"-key", NULL, PAR_INVALID},
        {"-out", NULL, PAR_INVALID},
        {"-sig", NULL, PAR_INVALID},
        {"-unsigned", NULL, PAR_INVALID},
        {"-ccsfile", NULL, PAR_REQUIRED},
        {"-dumpfile", NULL, PAR_REQUIRED}};

    const char *mode_m[] ={"sign", "gendata","catsig", "dump"};
    param_struct_t *params[] = {params_sign, params_gendata, params_catsig, params_dump};
    
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
        printf("Cannot recognize the command \"%s\".\nCommand \"sign/gendata/catsig\" is required.\n", argv[1]);
        return false;
    }

    unsigned int params_count = (unsigned)(sizeof(params_sign)/sizeof(params_sign[0]));
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

int main(int argc, char* argv[])
{
    printf("Welcome to PENGLAI sign_tool!\n");

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
    else if(mode == DUMP)
    {
        printf("DUMP not support now\n");
    }
	// else if(mode == DUMP)
    // {
    //     // dump metadata info
    //     if(dump_enclave_metadata(path[ELF], path[DUMPFILE]) == false)
    //     {
    //         printf("Failed to dump metadata info to file \"%s\".\n.", path[DUMPFILE]);
    //         goto clear_return;
    //     }
    //     printf("Succeed.\n");
    //     res = 0;
    //     goto clear_return;
    // }
    else if(mode == SIGN)
	{
        printf("SIGN image: %s, dtb: %s, keyfile: %s, output: %s, dumpfile(optional): %s\n", 
            path[IMAGE_FILE], path[DTB_FILE], path[KEY], path[OUTPUT], (path[DUMPFILE] ? path[DUMPFILE] : "--"));
        // load elf
        enclave_css_t enclave_css;
        if(load_secure_linux(&enclave_css) < 0){
            printf("ERROR: load secure linux failed!\n");
            goto clear_return;
        }

        // parse private key, sign and verify
        unsigned char *private_key = (unsigned char *)malloc(PRIVATE_KEY_SIZE);
        parse_priv_key_file(path[KEY], private_key, enclave_css.user_pub_key);
        sign_sec_linux((void *)(enclave_css.signature), enclave_css.enclave_hash, HASH_SIZE, private_key);

        printf("[sign_enclave] signature:\n");
        printHex(enclave_css.signature, SIGNATURE_SIZE);
        // generate_signature_DER("sig-der", enclave_css.signature);
        printf("[sign_enclave] enclave hash:\n");
        printHex(enclave_css.enclave_hash, HASH_SIZE);
        printf("[sign_enclave] private_key: \n");
        printHex(private_key, PRIVATE_KEY_SIZE);
        printf("[sign_enclave] public_key: \n");
        printHex(enclave_css.user_pub_key, PUBLIC_KEY_SIZE);
        printf("begin verify\n");
        int ret = verify_sec_linux((void *)(enclave_css.signature), enclave_css.enclave_hash, HASH_SIZE, enclave_css.user_pub_key);
        if(ret != 0){
            printf("ERROR: verify enclave_css struct failed!\n");
            goto clear_return;
        } else {
            printf("verify enclave's signature successfully.\n");
        }

        // generate out
        write_data_to_file(path[OUTPUT], "wb", (unsigned char *)&enclave_css, sizeof(enclave_css_t), 0);

        //dump
        // if(path[DUMPFILE] != NULL && dump_enclave_metadata(path[OUTPUT], path[DUMPFILE]) == false)
        // {
        //     printf("Failed to dump metadata info to file \"%s\".\n.", path[DUMPFILE]);
        //     goto clear_return;
        // }
	}
    else if(mode == GENDATA)
    {
        printf("GENDATA not support now\n");
    }
    else if(mode == CATSIG)
    {
        printf("CATSIG not support now\n");
    }
    // else if(mode == GENDATA)
    // {
    //     printf("GENDATA enclave: %s, output: %s, \n", path[ELF], path[OUTPUT]);
    //     // load elf
    //     enclave_css_t enclave_css;
    //     unsigned long meta_offset;
    //     if(load_secure_linux(path[ELF], &enclave_css, &meta_offset) < 0){
    //         printf("ERROR: load enclave failed!\n");
    //     }
    //     // output enclave hash
    //     write_data_to_file(path[OUTPUT], "wb", enclave_css.enclave_hash, HASH_SIZE, 0);
    // }
    // else if(mode == CATSIG)
    // {
    //     printf("CATSIG enclave: %s, keyfile: %s, output: %s, signatrue: %s, unsigned hash: %s, dumpfile(optional): %s\n", 
    //         path[ELF], path[KEY], path[OUTPUT], path[SIG], path[UNSIGNED], (path[DUMPFILE] ? path[DUMPFILE] : "--"));
    //     // load enclave to get meta_offset
    //     enclave_css_t enclave_css;
    //     unsigned long meta_offset;
    //     if(load_secure_linux(path[ELF], &enclave_css, &meta_offset) < 0){
    //         printf("ERROR: load enclave failed!\n");
    //         goto clear_return;
    //     }
    //     // parse public key, verify signature
    //     unsigned char *hash = (unsigned char *)malloc(HASH_SIZE);
    //     read_file_to_buf(path[UNSIGNED], hash, HASH_SIZE, 0);
    //     printf("hash:\n");
    //     printHex(hash, HASH_SIZE);
    //     unsigned char *public_key = (unsigned char *)malloc(PUBLIC_KEY_SIZE);
    //     parse_pub_key_file(path[KEY], public_key);
    //     printf("public key:\n");
    //     printHex(public_key, PUBLIC_KEY_SIZE);
    //     printf("publickey finish\n");
    //     unsigned char *signature = (unsigned char *)malloc(SIGNATURE_SIZE);
    //     parse_signature_DER(path[SIG], signature);
    //     printf("signature:\n");
    //     printHex(signature, SIGNATURE_SIZE);
    //     int ret = verify_enclave((struct signature_t *)signature, hash, HASH_SIZE, public_key);
    //     if(ret != 0){
    //         printf("ERROR: verify signature failed!\n");
    //         goto clear_return;
    //     }
    //     // append signature to eappfile
    //     copy_file(path[ELF], path[OUTPUT]);
    //     if(memcmp(enclave_css.enclave_hash, hash, HASH_SIZE) != 0){
    //         printf("ERROR: UNSIGNED hash is wrong.\n");
    //         goto clear_return;
    //     }
    //     memcpy(enclave_css.enclave_hash, hash, HASH_SIZE);
    //     memcpy(enclave_css.signature, signature, SIGNATURE_SIZE);
    //     memcpy(enclave_css.user_pub_key, public_key, PUBLIC_KEY_SIZE);
    //     update_metadata(path[OUTPUT], &enclave_css, meta_offset);
    //     //dump
    //     if(path[DUMPFILE] != NULL && dump_enclave_metadata(path[OUTPUT], path[DUMPFILE]) == false)
    //     {
    //         printf("Failed to dump metadata info to file \"%s\".\n.", path[DUMPFILE]);
    //         goto clear_return;
    //     }
    // }
    printf("Succeed.\n");

clear_return:
    return 0;
}
