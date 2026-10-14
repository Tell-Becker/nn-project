#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "custom_isa_ref.h"

static uint32_t random_state = UINT32_C(0x71c3a52d);

static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static uint64_t pack_lanes(const int8_t lanes[8])
{
    uint64_t packed = 0;

    for (unsigned lane = 0; lane < 8; lane++) {
        packed |= (uint64_t)(uint8_t)lanes[lane] << (lane * 8U);
    }
    return packed;
}

static int check_case(const char *name, uint64_t accumulator,
                      const int8_t left[8], const int8_t right[8],
                      uint64_t expected)
{
    uint64_t actual = xqmac8_reference(accumulator, pack_lanes(left),
                                       pack_lanes(right));

    if (actual != expected) {
        fprintf(stderr, "%s failed: expected 0x%016" PRIx64
                        ", got 0x%016" PRIx64 "\n",
                name, expected, actual);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const int8_t ones[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    static const int8_t signed_lanes[8] = {-128, 127, -1, 0, 1, -2, 64, -64};
    static const int8_t signed_weights[8] = {1, -1, -128, 127, -1, 2, -2, 2};
    static const int8_t extremes[8] = {-128, -128, -128, -128,
                                       -128, -128, -128, -128};
    int failures = 0;

    failures += check_case("simple sum", 5, ones, ones, 13);
    failures += check_case("signed lanes", 0, signed_lanes, signed_weights,
                          (uint64_t)(int64_t)-388);
    failures += check_case("extreme products", 0, extremes, extremes, 131072);
    failures += check_case("unsigned wraparound", UINT64_MAX, ones, ones, 7);

    for (unsigned test = 0; test < 1000; test++) {
        int8_t left[8];
        int8_t right[8];
        int64_t dot_product = 0;
        uint64_t accumulator = ((uint64_t)next_random() << 32) | next_random();

        for (unsigned lane = 0; lane < 8; lane++) {
            uint32_t left_byte = next_random() & 0xffU;
            uint32_t right_byte = next_random() & 0xffU;
            left[lane] = (int8_t)(left_byte < 128U
                                      ? (int32_t)left_byte
                                      : (int32_t)left_byte - 256);
            right[lane] = (int8_t)(right_byte < 128U
                                       ? (int32_t)right_byte
                                       : (int32_t)right_byte - 256);
            dot_product += (int64_t)left[lane] * (int64_t)right[lane];
        }

        failures += check_case("randomized", accumulator, left, right,
                               accumulator + (uint64_t)dot_product);
    }

    if (failures != 0) {
        fprintf(stderr, "Xqmac8 reference: %d test(s) failed\n", failures);
        return 1;
    }

    printf("Xqmac8 software reference passed: 4 directed + 1000 randomized cases\n");
    return 0;
}
