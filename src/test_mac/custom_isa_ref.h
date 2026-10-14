#ifndef CUSTOM_ISA_REF_H
#define CUSTOM_ISA_REF_H

#include <stdint.h>

static inline int32_t xqmac8_signed_lane(uint64_t packed, unsigned lane)
{
    uint32_t byte = (uint32_t)((packed >> (lane * 8U)) & UINT64_C(0xff));
    return byte < 128U ? (int32_t)byte : (int32_t)byte - 256;
}

static inline uint64_t xqmac8_reference(uint64_t accumulator,
                                       uint64_t packed_left,
                                       uint64_t packed_right)
{
    int64_t dot_product = 0;

    for (unsigned lane = 0; lane < 8; lane++) {
        dot_product += (int64_t)xqmac8_signed_lane(packed_left, lane) *
                       xqmac8_signed_lane(packed_right, lane);
    }

    return accumulator + (uint64_t)dot_product;
}

#endif
