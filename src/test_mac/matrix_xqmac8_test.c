#include <stdint.h>
#include <stdio.h>

#include "custom_isa.h"

#define MATRIX_ROWS 3
#define MATRIX_INNER 8
#define MATRIX_COLS 4

static int8_t matrix_left[MATRIX_ROWS * MATRIX_INNER] __attribute__((aligned(8)));
static int8_t matrix_right[MATRIX_INNER * MATRIX_COLS] __attribute__((aligned(8)));
static int32_t matrix_output[MATRIX_ROWS * MATRIX_COLS];
static int32_t matrix_reference[MATRIX_ROWS * MATRIX_COLS];

static uint32_t random_state = UINT32_C(0x3957a12d);

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

static uint64_t pack_i8x8(const int8_t values[MATRIX_INNER])
{
    uint64_t packed = 0;

    for (unsigned lane = 0; lane < MATRIX_INNER; lane++) {
        packed |= (uint64_t)(uint8_t)values[lane] << (lane * 8U);
    }
    return packed;
}

__attribute__((noinline))
static void matrix_multiply_scalar(const int8_t *left, const int8_t *right,
                                   int32_t *output)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            int32_t accumulator = 0;
            for (int inner = 0; inner < MATRIX_INNER; inner++) {
                accumulator += (int32_t)left[row * MATRIX_INNER + inner] *
                               (int32_t)right[inner * MATRIX_COLS + col];
            }
            output[row * MATRIX_COLS + col] = accumulator;
        }
    }
}

#if defined(USE_XQMAC8)
__attribute__((noinline))
static void matrix_multiply_xqmac8(const int8_t *left, const int8_t *right,
                                   int32_t *output)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        int8_t activation_lanes[MATRIX_INNER];
        for (int inner = 0; inner < MATRIX_INNER; inner++) {
            activation_lanes[inner] = left[row * MATRIX_INNER + inner];
        }
        uint64_t packed_activations = pack_i8x8(activation_lanes);

        for (int col = 0; col < MATRIX_COLS; col++) {
            int8_t weight_lanes[MATRIX_INNER];
            for (int inner = 0; inner < MATRIX_INNER; inner++) {
                weight_lanes[inner] = right[inner * MATRIX_COLS + col];
            }
            uint64_t packed_weights = pack_i8x8(weight_lanes);
            uint64_t accumulator =
                xqmac8_execute(0, packed_activations, packed_weights);
            output[row * MATRIX_COLS + col] = (int32_t)accumulator;
        }
    }
}
#endif

static void matrix_multiply_reference(const int8_t *left, const int8_t *right,
                                      int32_t *output)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            int64_t accumulator = 0;
            for (int inner = 0; inner < MATRIX_INNER; inner++) {
                accumulator +=
                    (int64_t)left[row * MATRIX_INNER + inner] *
                    (int64_t)right[inner * MATRIX_COLS + col];
            }
            output[row * MATRIX_COLS + col] = (int32_t)accumulator;
        }
    }
}

int main(void)
{
    for (unsigned index = 0; index < MATRIX_ROWS * MATRIX_INNER; index++) {
        matrix_left[index] = random_i8();
    }
    for (unsigned index = 0; index < MATRIX_INNER * MATRIX_COLS; index++) {
        matrix_right[index] = random_i8();
    }

    matrix_multiply_reference(matrix_left, matrix_right, matrix_reference);
#if defined(USE_XQMAC8)
    matrix_multiply_xqmac8(matrix_left, matrix_right, matrix_output);
    printf("Xqmac8 matrix kernel: ");
#elif defined(MATRIX_B1)
    matrix_multiply_scalar(matrix_left, matrix_right, matrix_output);
    printf("Scalar B1 matrix kernel: ");
#else
    matrix_multiply_scalar(matrix_left, matrix_right, matrix_output);
    printf("Scalar B0 matrix kernel: ");
#endif

    int failures = 0;
    int64_t checksum = 0;
    for (unsigned index = 0; index < MATRIX_ROWS * MATRIX_COLS; index++) {
        if (matrix_output[index] != matrix_reference[index]) {
            printf("mismatch at output %u: expected %d, got %d\n\r",
                   index, matrix_reference[index], matrix_output[index]);
            failures++;
        }
        checksum += matrix_output[index];
    }

    if (failures != 0) {
        printf("FAILED: %d output mismatch(es)\n\r", failures);
        return 1;
    }

    printf("%d x %d by %d x %d passed; checksum %lld\n\r",
           MATRIX_ROWS, MATRIX_INNER, MATRIX_INNER, MATRIX_COLS,
           (long long)checksum);
    return 0;
}
