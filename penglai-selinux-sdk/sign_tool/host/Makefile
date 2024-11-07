CC = gcc
AR = ar
ifeq ($(ARCH),riscv64)
	CC = riscv64-linux-gnu-gcc
	AR = riscv64-linux-gnu-ar
endif

INCLUDE_DIR = include/
SRC_DIR = src
CCFLAGS = -Wall -Werror -shared -fPIC -I $(INCLUDE_DIR)
SRCS = elf.c penglai-enclave.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
ARFLAGS = rcs

TARGET=libpenglai-enclave-host.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJS): %.o: $(addprefix $(SRC_DIR)/, %.c)
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
