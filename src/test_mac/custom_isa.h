#ifndef CUSTOM_ISA_H
#define CUSTOM_ISA_H

#include <stdint.h>

#define XQMAC8_OPCODE 0x0b
#define XQMAC8_FUNCT3 0
#define XQMAC8_FUNCT7 0

#define XQMAC8_STRINGIFY_INNER(value) #value
#define XQMAC8_STRINGIFY(value) XQMAC8_STRINGIFY_INNER(value)

#if defined(__riscv)
static inline uint64_t xqmac8_execute(uint64_t accumulator,
                                      uint64_t packed_left,
                                      uint64_t packed_right)
{
    __asm__ volatile (
        ".insn r " XQMAC8_STRINGIFY(XQMAC8_OPCODE) ", "
        XQMAC8_STRINGIFY(XQMAC8_FUNCT3) ", "
        XQMAC8_STRINGIFY(XQMAC8_FUNCT7) ", %0, %1, %2"
        : "+r"(accumulator)
        : "r"(packed_left), "r"(packed_right));
    return accumulator;
}
#endif

#undef XQMAC8_STRINGIFY
#undef XQMAC8_STRINGIFY_INNER

#endif
