# 6502 Emulator Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I$(SRC_DIR)
LDFLAGS =

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Exclude main.o and scrap.o for test builds (tests have their own main)
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/scrap.o,$(OBJS))

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

# Main target
TARGET = $(BUILD_DIR)/emu6502

.PHONY: all clean test run

all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link main executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# Build test executables (link test file with library objects, not main.o)
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB_OBJS) -o $@

# Build all tests
test: $(TEST_BINS)
	@echo "Running tests..."
	@for test in $(TEST_BINS); do \
		echo "\n=== Running $$test ==="; \
		./$$test || exit 1; \
	done
	@echo "\n✓ All tests passed"

# Run the emulator
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Debug: show variables
debug:
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
	@echo "LIB_OBJS: $(LIB_OBJS)"
	@echo "TEST_SRCS: $(TEST_SRCS)"
	@echo "TEST_BINS: $(TEST_BINS)"