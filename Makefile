CC      := gcc
INCDIRS := -Iinclude
CFLAGS  := -g -O0 -Wall -Wextra $(INCDIRS)

# 测试专用（建议开启 sanitizer）
SANITIZE    := -fsanitize=address,undefined -fno-omit-frame-pointer
TEST_CFLAGS := $(CFLAGS) $(SANITIZE)
TEST_LDFLAGS:= $(SANITIZE)

BUILD_DIR := build
TEST_DIR  := $(BUILD_DIR)/test

# 被测源码（后续加 rbtree/hash 时只需在这里追加）
SRC_ALLOC  := src/allocator/kvs_alloc.c
SRC_ARRAY  := src/engine/kvs_array.c
SRC_RBTREE := src/engine/kvs_rbtree.c
SRC_HASH   := src/engine/kvs_hash.c
# 统一引擎源码集合（后续继续加）
SRC_ENGINE := $(SRC_ARRAY) $(SRC_RBTREE) $(SRC_HASH)

# 单元测试源文件列表（后续新增测试文件只要往这行加）
UNIT_TESTS := \
	test/unit/test_array.c \
	test/unit/test_rbtree.c \
	test/unit/test_hash.c

# 将 test/unit/test_xxx.c 映射为 build/test/test_xxx
UNIT_BINS := $(patsubst test/unit/%.c,$(TEST_DIR)/%,$(UNIT_TESTS))

.PHONY: all test test_unit clean

all:
	@echo "Targets: make test | make clean"

test: test_unit

test_unit: $(UNIT_BINS)
	@set -e; for t in $(UNIT_BINS); do echo "[RUN] $$t"; $$t; done
	@echo "[OK] all unit tests passed."

# 通用规则：把 test/unit/xxx.c 编译成 build/test/xxx
$(TEST_DIR)/%: test/unit/%.c $(SRC_ENGINE) $(SRC_ALLOC) | $(TEST_DIR)
	$(CC) $(TEST_CFLAGS) $^ -o $@ $(TEST_LDFLAGS)

$(TEST_DIR):
	mkdir -p $(TEST_DIR)

clean:
	rm -rf $(BUILD_DIR)
