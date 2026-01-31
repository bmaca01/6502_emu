#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "cpu.h"
#include "util.h"
#include <stdio.h>

#define MAX_FAILED_TESTS 256

/* Test state - defined in test_common.c */
extern int check_failures;
extern int failed_test_count;
extern int tests_run;
extern const char* failed_tests[];
extern const char* current_test_name;
extern int current_test_failed;

/* Test macros */
#define TEST(name) static void name(void)

#define RUN_TEST(name) do { \
    current_test_name = #name; \
    current_test_failed = 0; \
    tests_run++; \
    printf("  %-40s", #name); \
    name(); \
    if (!current_test_failed) { \
        printf(" ✓\n"); \
    } else { \
        printf(" ✗\n"); \
        if (failed_test_count < MAX_FAILED_TESTS) { \
            failed_tests[failed_test_count++] = current_test_name; \
        } \
    } \
} while(0)

#define CHECK(condition, ...) do { \
    if (!(condition)) { \
        check_failures++; \
        current_test_failed = 1; \
        const char* _msg = "" __VA_ARGS__; \
        if (_msg[0] != '\0') { \
            printf("\n    FAIL: %s:%d: %s", __FILE__, __LINE__, _msg); \
        } else { \
            printf("\n    FAIL: %s:%d: %s", __FILE__, __LINE__, #condition); \
        } \
    } \
} while(0)

#define CHECK_EQ(actual, expected) do { \
    int _a = (actual); \
    int _e = (expected); \
    if (_a != _e) { \
        check_failures++; \
        current_test_failed = 1; \
        printf("\n    FAIL: %s:%d: expected 0x%04X, got 0x%04X", \
               __FILE__, __LINE__, _e, _a); \
    } \
} while(0)

#define check_pc(cpu, expected) \
    CHECK_EQ(cpu_get_pc(cpu), (expected))

/* Helper functions */
CPU* setup_cpu(void);
void check_flags(CPU* cpu, int n_expected, int z_expected);
void print_test_summary(void);
void reset_test_state(void);

#endif
