TARGET = test_mac
RISCV_CC ?= $(if $(wildcard $(HOME)/riscv/bin/riscv64-unknown-elf-gcc),$(HOME)/riscv/bin/riscv64-unknown-elf-gcc,riscv64-unknown-elf-gcc)
OBJDUMP ?= $(if $(wildcard $(HOME)/riscv/bin/riscv64-unknown-elf-objdump),$(HOME)/riscv/bin/riscv64-unknown-elf-objdump,riscv64-unknown-elf-objdump)
SPIKE ?= $(if $(wildcard $(HOME)/riscv/riscv-isa-sim/build/spike),$(HOME)/riscv/riscv-isa-sim/build/spike,spike)
SPIKE_LIBDIR ?= $(HOME)/riscv/riscv-isa-sim/build

COMMON_CFLAGS = -gdwarf-2 -march=rv64imc_zicsr -mabi=lp64 -mcmodel=medany \
	-nostdlib -static -lm -fno-tree-loop-distribute-patterns \
	-ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS_B0 ?= $(COMMON_CFLAGS) -O0
CFLAGS_B1 ?= $(COMMON_CFLAGS) -O2
SOURCES = $(TARGET).c ../common/crt.S ../common/syscalls.c

.PHONY: all xqmac8-host-test xqmac8-spike-test xqmac8-matrix-test clean

all: $(TARGET) $(TARGET)_B1 $(TARGET).objdump $(TARGET)_B1.objdump

xqmac8-host-test: custom_isa_ref_test
	./custom_isa_ref_test

xqmac8-spike-test: custom_isa_spike_test
	LD_LIBRARY_PATH="$(SPIKE_LIBDIR):$${LD_LIBRARY_PATH:-}" \
		$(SPIKE) --isa=rv64imc_zicsr --extension=xqmac $<

xqmac8-matrix-test: matrix_xqmac8_B0 matrix_xqmac8_B1 matrix_xqmac8_X1
	$(SPIKE) --isa=rv64imc_zicsr matrix_xqmac8_B0
	$(SPIKE) --isa=rv64imc_zicsr matrix_xqmac8_B1
	LD_LIBRARY_PATH="$(SPIKE_LIBDIR):$${LD_LIBRARY_PATH:-}" \
		$(SPIKE) --isa=rv64imc_zicsr --extension=xqmac matrix_xqmac8_X1

matrix_xqmac8_B0: matrix_xqmac8_test.c ../common/crt.S \
		../common/syscalls.c Makefile
	$(RISCV_CC) $(CFLAGS_B0) -T../common/test.ld -I../common \
		-o $@ matrix_xqmac8_test.c ../common/crt.S ../common/syscalls.c

matrix_xqmac8_B1: matrix_xqmac8_test.c ../common/crt.S \
		../common/syscalls.c Makefile
	$(RISCV_CC) $(CFLAGS_B1) -DMATRIX_B1 -T../common/test.ld -I../common \
		-o $@ matrix_xqmac8_test.c ../common/crt.S ../common/syscalls.c

matrix_xqmac8_X1: matrix_xqmac8_test.c custom_isa.h ../common/crt.S \
		../common/syscalls.c Makefile
	$(RISCV_CC) $(CFLAGS_B1) -DUSE_XQMAC8 -T../common/test.ld -I../common \
		-o $@ matrix_xqmac8_test.c ../common/crt.S ../common/syscalls.c

custom_isa_spike_test: custom_isa_spike_test.c custom_isa.h \
		../common/crt.S ../common/syscalls.c Makefile
	$(RISCV_CC) $(CFLAGS_B1) -T../common/test.ld -I../common \
		-o $@ custom_isa_spike_test.c ../common/crt.S ../common/syscalls.c

custom_isa_ref_test: custom_isa_ref_test.c custom_isa_ref.h Makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -o $@ custom_isa_ref_test.c

$(TARGET): $(SOURCES) Makefile
	$(RISCV_CC) $(CFLAGS_B0) -T../common/test.ld -I../common -o $@ $(SOURCES)

$(TARGET)_B1: $(SOURCES) Makefile
	$(RISCV_CC) $(CFLAGS_B1) -T../common/test.ld -I../common -o $@ $(SOURCES)

$(TARGET).objdump: $(TARGET)
	$(OBJDUMP) -S -d $< > $@

$(TARGET)_B1.objdump: $(TARGET)_B1
	$(OBJDUMP) -S -d $< > $@

clean:
	rm -f $(TARGET) $(TARGET)_B1 $(TARGET).objdump* $(TARGET)_B1.objdump* \
		$(TARGET).memfile custom_isa_ref_test custom_isa_spike_test \
		matrix_xqmac8_B0 matrix_xqmac8_B1 matrix_xqmac8_X1
