CC=riscv64-linux-gnu-gcc

HOST=remote

all:
	make -C gm
	$(CC) -o $(HOST) -static -I ../lib/include/ -I ./gm/include/ remote.c util.c gm/sm2.a gm/miracl.a -lpthread

clean:
	make -C gm clean
	rm -f *.o $(HOST)
