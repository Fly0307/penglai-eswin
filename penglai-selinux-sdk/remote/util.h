#ifndef _ST_UTIL_H
#define _ST_UTIL_H

#include <stddef.h>

#define USAGE_STRING \
    "\nUsage: host <commands> [options]...\n"\
    "Commands:\n"\
    "   run                    Sign the enclave using the private key\n"\
    "   attest                 Generate enclave signing material to be signed\n"\
    "Options:\n"\
    "   -image                 Specify the Linux image file to be loaded and run\n"\
    "                          It is a required option for \"run\"\n"\
    "   -imageaddr             Specify the load address of the Linux image\n"\
    "                          It is a required option for \"run\"\n"\
    "   -dtb                   Specify the device tree file to be loaded and run\n"\
    "                          It is a required option for \"run\"\n"\
    "   -dtbaddr               Specify the load address of the device tree\n"\
    "                          It is a required option for \"run\"\n"\
    "   -nonce                 Specify the challenging nonce\n" \
    "                          It is a required option for \"attest\"\n" \
    "Run \"host -help\" to get this help and exit.\n"

typedef enum _command_mode_t
{
    RUN = 0,
    ATTEST,
} command_mode_t;

typedef enum _file_path_t
{
    IMAGE_FILE = 0,
    IMAGE_LDADDR,
    DTB_FILE,
    DTB_LDADDR,
    NONCE
} file_path_t;

void printHex(unsigned char *c, int n);
int get_file_size(const char *filename);
int read_file_to_buf(const char *filename, unsigned char *buffer, size_t bsize, long offset);
int write_data_to_file(const char *filename, const char *modes, unsigned char *buf, size_t bsize, long offset);
int copy_file(const char *source_path, const char *dest_path);

#endif
