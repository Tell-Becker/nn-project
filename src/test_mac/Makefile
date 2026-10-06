TARGET = test_mac
RISCV_CC ?= riscv64-unknown-elf-gcc
OBJDUMP ?= riscv64-unknown-elf-objdump

COMMON_CFLAGS = -gdwarf-2 -march=rv64imc_zicsr -mabi=lp64 -mcmodel=medany \
	-nostdlib -static -lm -fno-tree-loop-distribute-patterns \
	-ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS_B0 ?= $(COMMON_CFLAGS) -O0
CFLAGS_B1 ?= $(COMMON_CFLAGS) -O2
SOURCES = $(TARGET).c ../common/crt.S ../common/syscalls.c

.PHONY: all clean

all: $(TARGET) $(TARGET)_B1 $(TARGET).objdump $(TARGET)_B1.objdump

$(TARGET): $(SOURCES) Makefile
	$(RISCV_CC) $(CFLAGS_B0) -T../common/test.ld -I../common -o $@ $(SOURCES)

$(TARGET)_B1: $(SOURCES) Makefile
	$(RISCV_CC) $(CFLAGS_B1) -T../common/test.ld -I../common -o $@ $(SOURCES)

$(TARGET).objdump: $(TARGET)
	$(OBJDUMP) -S -d $< > $@

$(TARGET)_B1.objdump: $(TARGET)_B1
	$(OBJDUMP) -S -d $< > $@

clean:
	rm -f $(TARGET) $(TARGET)_B1 $(TARGET).objdump* $(TARGET)_B1.objdump* $(TARGET).memfile
