##
## Author: Shang Qingyu
## Description:
## 	 compile sign tool need openssl 3.0.0 installation.

CC = gcc
CUR_SOURCE = ${wildcard *.c}

GM_LIBS = gm/sm2.a gm/miracl.a
GM_LIB_INCLUDE_DIR = gm/include
HOST_LIB = host/libpenglai-enclave-host.a
HOST_LIB_INCLUDE_DIR = host/include/

CFLAGS += -I $(HOST_LIB_INCLUDE_DIR) -I $(GM_LIB_INCLUDE_DIR)

all: penglai_sign

$(GM_LIBS):
	make -C gm

$(HOST_LIB):
	make -C host

penglai_sign: $(CUR_SOURCE) $(GM_LIBS) $(HOST_LIB)
	$(CC) $(CFLAGS) -o $@ $^ -L /usr/local/lib -lssl -lcrypto -ldl -lpthread

clean:
	make -C gm clean
	make -C host clean
	rm -f *.o penglai_sign
