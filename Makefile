CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
BUILD := build

.PHONY: test bench clean all

all: test

test: $(BUILD)/runtime_test
	./$(BUILD)/runtime_test

$(BUILD)/runtime_test: src/binary_codec.c src/check_sum.c test/runtime_test.c include/binary_codec.h include/check_sum.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -o $@ src/binary_codec.c src/check_sum.c test/runtime_test.c

clean:
	rm -rf $(BUILD)
