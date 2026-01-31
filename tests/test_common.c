#include "test_common.h"
#include "memory.h"

/* Test state variables */
int check_failures = 0;
int failed_test_count = 0;
int tests_run = 0;
const char* failed_tests[MAX_FAILED_TESTS];
const char* current_test_name = NULL;
int current_test_failed = 0;

/* Reset test state for each test file */
void reset_test_state(void) {
    check_failures = 0;
    failed_test_count = 0;
    tests_run = 0;
    current_test_name = NULL;
    current_test_failed = 0;
}

/* Setup a CPU with reset vector pointing to $0200 */
CPU* setup_cpu(void) {
    Memory* mem = memory_create();
    /* Set reset vector to 0x0200 */
    memory_write(mem, 0xFFFC, 0x00);
    memory_write(mem, 0xFFFD, 0x02);
    return cpu_create(mem);
}

/* Check N and Z flags */
void check_flags(CPU* cpu, int n_expected, int z_expected) {
    uint8_t status = cpu_get_status(cpu);
    int n_actual = !!(status & FLAG_N);
    int z_actual = !!(status & FLAG_Z);
    if (n_expected != n_actual) {
        CHECK(0 && "N flag mismatch");
    }
    if (z_expected != z_actual) {
        CHECK(0 && "Z flag mismatch");
    }
}

/* Print test summary */
void print_test_summary(void) {
    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Passed: %d\n", tests_run - failed_test_count);
    printf("Failed: %d\n", failed_test_count);

    if (failed_test_count > 0) {
        printf("\nFailed tests:\n");
        for (int i = 0; i < failed_test_count; i++) {
            printf("  - %s\n", failed_tests[i]);
        }
        printf("\n");
    } else {
        printf("\nAll tests passed!\n\n");
    }
}
