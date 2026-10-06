#include <stdio.h>
#include <stdint.h>

#define DOT_MAX_LENGTH 257
#define MATRIX_ROWS 3
#define MATRIX_INNER 5
#define MATRIX_COLS 4

static int32_t dot_product_i8(const int8_t *left, const int8_t *right, int length)
{
    int32_t accumulator = 0;
    for (int index = 0; index < length; index++) {
        accumulator += (int32_t)left[index] * (int32_t)right[index];
    }
    return accumulator;
}

static int64_t reference_dot_product_i8(const int8_t *left, const int8_t *right,
                                        int length)
{
    int64_t accumulator = 0;
    for (int index = 0; index < length; index++) {
        accumulator += (int64_t)left[index] * (int64_t)right[index];
    }
    return accumulator;
}

static void matrix_multiply_i8(const int8_t *left, const int8_t *right,
                               int32_t *output, int rows, int inner, int cols)
{
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int32_t accumulator = 0;
            for (int index = 0; index < inner; index++) {
                accumulator += (int32_t)left[row * inner + index] *
                               (int32_t)right[index * cols + col];
            }
            output[row * cols + col] = accumulator;
        }
    }
}

static void reference_matrix_multiply_i8(const int8_t *left, const int8_t *right,
                                         int32_t *output, int rows, int inner, int cols)
{
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int64_t accumulator = 0;
            for (int index = 0; index < inner; index++) {
                accumulator += (int64_t)left[row * inner + index] *
                               (int64_t)right[index * cols + col];
            }
            output[row * cols + col] = (int32_t)accumulator;
        }
    }
}

static uint32_t random_state = 0x6d2b79f5U;

static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static int8_t random_i8(void)
{
    return (int8_t)((int32_t)(next_random() & 0xffU) - 128);
}

int main(void) {
    const int8_t example_left[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const int8_t example_right[8] = {1, -1, 1, -1, 1, -1, 1, -1};
    const int8_t edge_left[8] = {-128, 127, -128, 127, -1, 0, 1, -1};
    const int8_t edge_right[8] = {-128, 127, 127, -128, -128, 127, -128, 127};
    int8_t dot_left[DOT_MAX_LENGTH];
    int8_t dot_right[DOT_MAX_LENGTH];
    const int dot_lengths[] = {1, 8, 31, 128, DOT_MAX_LENGTH};
    int8_t matrix_left[MATRIX_ROWS * MATRIX_INNER];
    int8_t matrix_right[MATRIX_INNER * MATRIX_COLS];
    int32_t matrix_output[MATRIX_ROWS * MATRIX_COLS];
    int32_t matrix_reference[MATRIX_ROWS * MATRIX_COLS];
    int failures = 0;

    printf("MAC result: %d\n\r",
           dot_product_i8(example_left, example_right, 8));

    if (dot_product_i8(edge_left, edge_right, 8) != -126 ||
        reference_dot_product_i8(edge_left, edge_right, 8) != -126) {
        printf("Dot boundary test failed\n\r");
        failures++;
    }

    for (int test = 0; test < 64; test++) {
        for (int index = 0; index < DOT_MAX_LENGTH; index++) {
            dot_left[index] = random_i8();
            dot_right[index] = random_i8();
        }

        int length = dot_lengths[test % (sizeof(dot_lengths) / sizeof(dot_lengths[0]))];
        int32_t actual = dot_product_i8(dot_left, dot_right, length);
        int64_t expected = reference_dot_product_i8(dot_left, dot_right, length);
        if ((int64_t)actual != expected) {
            printf("Dot test failed: test %d, length %d\n\r", test, length);
            failures++;
        }
    }

    for (int index = 0; index < MATRIX_ROWS * MATRIX_INNER; index++) {
        matrix_left[index] = random_i8();
    }
    for (int index = 0; index < MATRIX_INNER * MATRIX_COLS; index++) {
        matrix_right[index] = random_i8();
    }

    matrix_multiply_i8(matrix_left, matrix_right, matrix_output,
                       MATRIX_ROWS, MATRIX_INNER, MATRIX_COLS);
    reference_matrix_multiply_i8(matrix_left, matrix_right, matrix_reference,
                                 MATRIX_ROWS, MATRIX_INNER, MATRIX_COLS);
    for (int index = 0; index < MATRIX_ROWS * MATRIX_COLS; index++) {
        if (matrix_output[index] != matrix_reference[index]) {
            printf("Matrix test failed: output index %d\n\r", index);
            failures++;
        }
    }

    if (failures != 0) {
        printf("FAILED: %d check(s)\n\r", failures);
        return 1;
    }

    printf("Dot product tests passed: boundary + 64 deterministic cases\n\r");
    printf("Matrix multiply passed: %d x %d by %d x %d\n\r",
           MATRIX_ROWS, MATRIX_INNER, MATRIX_INNER, MATRIX_COLS);
    return 0;
}
