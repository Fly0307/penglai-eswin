CC=riscv64-linux-gnu-gcc

HOST=host

all:
	$(CC) -o $(HOST) -static -I ../lib/include/  host.c util.c ../lib/libpenglai-enclave-host.a  -lpthread

clean:
	rm -f *.o $(HOST)
