#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "custom_isa.h"

#define TOKENS 197
#define INPUT_FEATURES 192
#define OUTPUT_FEATURES 192

static int8_t activations[TOKENS * INPUT_FEATURES] __attribute__((aligned(8)));
static int8_t weights[INPUT_FEATURES * OUTPUT_FEATURES] __attribute__((aligned(8)));
static int8_t transposed_weights[OUTPUT_FEATURES * INPUT_FEATURES]
    __attribute__((aligned(8)));
static int32_t output[TOKENS * OUTPUT_FEATURES];
static int32_t reference[TOKENS * OUTPUT_FEATURES];

static uint32_t random_state = UINT32_C(0x6d2b79f5);

static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static int8_t random_i8(void)
{
    uint32_t byte = next_random() & 0xffU;
    int32_t value = byte < 128U ? (int32_t)byte : (int32_t)byte - 256;
    return (int8_t)value;
}

static void prepare_data(void)
{
    for (int index = 0; index < TOKENS * INPUT_FEATURES; index++) {
        activations[index] = random_i8();
    }
    for (int index = 0; index < INPUT_FEATURES * OUTPUT_FEATURES; index++) {
        weights[index] = random_i8();
    }
    for (int input = 0; input < INPUT_FEATURES; input++) {
        for (int output_feature = 0; output_feature < OUTPUT_FEATURES;
             output_feature++) {
            transposed_weights[output_feature * INPUT_FEATURES + input] =
                weights[input * OUTPUT_FEATURES + output_feature];
        }
    }
}

static void calculate_reference(void)
{
    for (int token = 0; token < TOKENS; token++) {
        for (int output_feature = 0; output_feature < OUTPUT_FEATURES;
             output_feature++) {
            int64_t accumulator = 0;
            for (int input = 0; input < INPUT_FEATURES; input++) {
                accumulator +=
                    (int64_t)activations[token * INPUT_FEATURES + input] *
                    (int64_t)weights[input * OUTPUT_FEATURES + output_feature];
            }
            reference[token * OUTPUT_FEATURES + output_feature] =
                (int32_t)accumulator;
        }
    }
}

__attribute__((noinline))
static void matrix_multiply_scalar_transposed(void)
{
    for (int token = 0; token < TOKENS; token++) {
        for (int output_feature = 0; output_feature < OUTPUT_FEATURES;
             output_feature++) {
            int32_t accumulator = 0;
            for (int input = 0; input < INPUT_FEATURES; input++) {
                accumulator +=
                    (int32_t)activations[token * INPUT_FEATURES + input] *
                    (int32_t)transposed_weights[
                        output_feature * INPUT_FEATURES + input];
            }
            output[token * OUTPUT_FEATURES + output_feature] = accumulator;
        }
    }
}

#if defined(USE_XQMAC8)
static uint64_t load_packed_i8x8(const int8_t *values)
{
    uint64_t packed;
    __builtin_memcpy(&packed, values, sizeof(packed));
    return packed;
}

__attribute__((noinline))
static void matrix_multiply_xqmac8_transposed(void)
{
    for (int token = 0; token < TOKENS; token++) {
        for (int output_feature = 0; output_feature < OUTPUT_FEATURES;
             output_feature++) {
            uint64_t accumulator = 0;
            for (int input = 0; input < INPUT_FEATURES; input += 8) {
                uint64_t packed_activations = load_packed_i8x8(
                    &activations[token * INPUT_FEATURES + input]);
                uint64_t packed_weights = load_packed_i8x8(
                    &transposed_weights[output_feature * INPUT_FEATURES + input]);
                accumulator = xqmac8_execute(accumulator,
                                              packed_activations,
                                              packed_weights);
            }
            output[token * OUTPUT_FEATURES + output_feature] =
                (int32_t)accumulator;
        }
    }
}
#endif

int main(void)
{
    prepare_data();
    calculate_reference();

#if defined(USE_XQMAC8)
    matrix_multiply_xqmac8_transposed();
    printf("Configuration: X1 (B1 + Xqmac8)\n\r");
#else
    matrix_multiply_scalar_transposed();
#if defined(MATRIX_B1)
    printf("Configuration: B1 (scalar, -O2)\n\r");
#else
    printf("Configuration: B0 (scalar, -O0)\n\r");
#endif
#endif

    unsigned mismatches = 0;
    int64_t checksum = 0;
    for (int index = 0; index < TOKENS * OUTPUT_FEATURES; index++) {
        if (output[index] != reference[index]) {
            mismatches++;
        }
        checksum += output[index];
    }
    if (mismatches != 0) {
        printf("Projection failed: %u mismatches\n\r", mismatches);
        return 1;
    }

    printf("Projection passed: %d x %d by %d x %d; "
           "%d MACs; checksum %lld\n\r",
           TOKENS, INPUT_FEATURES, INPUT_FEATURES, OUTPUT_FEATURES,
           TOKENS * INPUT_FEATURES * OUTPUT_FEATURES,
           (long long)checksum);
    return 0;
}
