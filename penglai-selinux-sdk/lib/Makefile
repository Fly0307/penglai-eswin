CC = riscv64-linux-gnu-gcc
INCLUDE_DIR = include/
SRC_DIR = src
CCFLAGS = -Wall -Werror -shared -fPIC -I $(INCLUDE_DIR)
SRCS = elf.c penglai-enclave.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
AR = riscv64-linux-gnu-ar
ARFLAGS = rcs

TARGET=libpenglai-enclave-host.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJS): %.o: $(addprefix $(SRC_DIR)/, %.c)
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
