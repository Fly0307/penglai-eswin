#include "penglai-enclave.h"

void PLenclave_init(struct PLenclave *PLenclave)
{
    PLenclave->bin_file = NULL;
    PLenclave->dtb_file = NULL;
    PLenclave->css_file = NULL;
    PLenclave->fd = open(PENGLAI_ENCLAVE_DEV_PATH, O_RDWR);
    if (PLenclave->fd < 0)
    {
        fprintf(stderr, "LIB: cannot open enclave dev\n");
    }
}

void PLenclave_finalize(struct PLenclave *PLenclave)
{
    if (PLenclave->fd >= 0)
        close(PLenclave->fd);
}

int PLenclave_load_and_run(struct PLenclave *PLenclave, 
                           struct elf_args *u_bin_file, unsigned long bin_loadaddr, 
                           struct elf_args *u_dtb_file, unsigned long dtb_loadaddr, struct elf_args *u_css_file)
{
    int ret = 0;
    if (!u_bin_file)
    {
        fprintf(stderr, "LIB: bin_file is not existed\n");
        return -1;
    }

    PLenclave->bin_file = u_bin_file;
    PLenclave->user_param.bin_ptr = (unsigned long)u_bin_file->ptr;
    PLenclave->user_param.bin_size = u_bin_file->size;
    PLenclave->user_param.bin_loadaddr = bin_loadaddr;

    if (PLenclave->user_param.bin_ptr == 0 || PLenclave->user_param.bin_size <= 0)
    {
        fprintf(stderr, "LIB: ioctl create enclave: bin_ptr is NULL\n");
        return -1;
    }

    if (!u_dtb_file)
    {
        fprintf(stderr, "LIB: bin_file is not existed\n");
        return -1;
    }

    PLenclave->dtb_file = u_dtb_file;
    PLenclave->user_param.dtb_ptr = (unsigned long)u_dtb_file->ptr;
    PLenclave->user_param.dtb_size = u_dtb_file->size;
    PLenclave->user_param.dtb_loadaddr = dtb_loadaddr;

    if (PLenclave->user_param.dtb_ptr == 0 || PLenclave->user_param.dtb_size <= 0)
    {
        fprintf(stderr, "LIB: ioctl create enclave: dtb_ptr is NULL\n");
        return -1;
    }

    PLenclave->css_file = u_css_file;
    PLenclave->user_param.css_ptr = (unsigned long)u_css_file->ptr;
    PLenclave->user_param.css_size = u_css_file->size;
    if (PLenclave->user_param.css_ptr == 0 || PLenclave->user_param.css_size <= 0)
    {
        fprintf(stderr, "LIB: ioctl create enclave: dtb_ptr is NULL\n");
        return -1;
    }

    ret = ioctl(PLenclave->fd, PENGLAI_IOC_LOAD_AND_RUN_LINUX, &(PLenclave->user_param));
    if (ret < 0)
    {
        fprintf(stderr, "LIB: ioctl create enclave is failed\n");
        return -1;
    }

    return 0;
}

int PLenclave_attest(struct PLenclave *PLenclave, unsigned long nonce)
{
    int ret = 0;

    PLenclave->attest_param.nonce = nonce;

    ret = ioctl(PLenclave->fd, PENGLAI_IOC_ATTEST_LINUX, &(PLenclave->attest_param));
    if (ret < 0)
    {
        fprintf(stderr, "LIB: ioctl attest enclave is failed\n");
        return -1;
    }

    return 0;
}
