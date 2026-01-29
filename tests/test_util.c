#include "util.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("  %-40s", #name); \
    name(); \
    printf(" ✓\n"); \
} while(0)

TEST(test_encode_op) {
    opcode_t op = NOP;
    addr_mode_t am = IMPL;
    uint8_t res = encode_op(op, am);

    assert(res == 0xEA);
}

int main(void) {
    RUN_TEST(test_encode_op);
    return 0;
}
