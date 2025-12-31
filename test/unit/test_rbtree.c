// test/unit/test_rbtree.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "engine/kvs_rbtree.h"

// ---------- 简单断言宏 ----------
#define EXPECT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while(0)

#define EXPECT_EQ_INT(a, b) do { \
    int _va = (a), _vb = (b); \
    if (_va != _vb) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_EQ_INT(%s=%d, %s=%d)\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        exit(1); \
    } \
} while(0)

#define EXPECT_EQ_STR(a, b) do { \
    const char *_sa = (a); \
    const char *_sb = (b); \
    if (_sa == NULL || _sb == NULL || strcmp(_sa, _sb) != 0) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_EQ_STR(%s=\"%s\", %s=\"%s\")\n", \
                __FILE__, __LINE__, #a, (_sa?_sa:"(null)"), #b, (_sb?_sb:"(null)")); \
        exit(1); \
    } \
} while(0)

// Fisher–Yates 洗牌，用于打乱插入顺序
static void shuffle_ints(int *a, int n, unsigned int seed) {
    srand(seed);
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static void test_basic_api(void) {
    kvs_rbtree_t t;
    memset(&t, 0, sizeof(t));

    EXPECT_EQ_INT(kvs_rbtree_create(&t), 0);

    // 空树：get/exist/del
    EXPECT_TRUE(kvs_rbtree_get(&t, "no_such_key") == NULL);
    EXPECT_EQ_INT(kvs_rbtree_exist(&t, "no_such_key"), 1);
    EXPECT_EQ_INT(kvs_rbtree_del(&t, "no_such_key"), 1);

    // set -> exist/get
    EXPECT_EQ_INT(kvs_rbtree_set(&t, "k1", "v1"), 0);
    EXPECT_EQ_INT(kvs_rbtree_exist(&t, "k1"), 0);
    EXPECT_EQ_STR(kvs_rbtree_get(&t, "k1"), "v1");

    // 重复 set：返回已存在(1)，且不覆盖
    EXPECT_EQ_INT(kvs_rbtree_set(&t, "k1", "v1_new"), 1);
    EXPECT_EQ_STR(kvs_rbtree_get(&t, "k1"), "v1"); // 仍为旧值

    // mod：更新 value
    EXPECT_EQ_INT(kvs_rbtree_mod(&t, "k1", "v1_mod"), 0);
    EXPECT_EQ_STR(kvs_rbtree_get(&t, "k1"), "v1_mod");

    // del：删除后不可见
    EXPECT_EQ_INT(kvs_rbtree_del(&t, "k1"), 0);
    EXPECT_EQ_INT(kvs_rbtree_exist(&t, "k1"), 1);
    EXPECT_TRUE(kvs_rbtree_get(&t, "k1") == NULL);

    // destroy
    kvs_rbtree_destory(&t);
}

static void test_mass_insert_modify_delete(void) {
    kvs_rbtree_t t;
    memset(&t, 0, sizeof(t));
    EXPECT_EQ_INT(kvs_rbtree_create(&t), 0);

    const int N = 2000;
    int *idx = (int*)malloc(sizeof(int) * N);
    EXPECT_TRUE(idx != NULL);

    for (int i = 0; i < N; ++i) idx[i] = i;
    shuffle_ints(idx, N, 20251231u);

    // 1) 随机顺序插入 N 个 key
    for (int k = 0; k < N; ++k) {
        int i = idx[k];
        char key[64], val[64];
        snprintf(key, sizeof(key), "key_%06d", i);
        snprintf(val, sizeof(val), "val_%06d", i);

        int rc = kvs_rbtree_set(&t, key, val);
        EXPECT_EQ_INT(rc, 0);

        // 立即校验
        EXPECT_EQ_INT(kvs_rbtree_exist(&t, key), 0);
        EXPECT_EQ_STR(kvs_rbtree_get(&t, key), val);
    }

    // 2) 重复插入一部分 key，必须返回 1 且不覆盖
    for (int i = 0; i < N; i += 10) {
        char key[64];
        snprintf(key, sizeof(key), "key_%06d", i);

        int rc = kvs_rbtree_set(&t, key, "SHOULD_NOT_OVERWRITE");
        EXPECT_EQ_INT(rc, 1);

        char expect_old[64];
        snprintf(expect_old, sizeof(expect_old), "val_%06d", i);
        EXPECT_EQ_STR(kvs_rbtree_get(&t, key), expect_old);
    }

    // 3) 修改一半 key 的 value
    for (int i = 0; i < N; i += 2) {
        char key[64], newv[64];
        snprintf(key, sizeof(key), "key_%06d", i);
        snprintf(newv, sizeof(newv), "val2_%06d", i);

        int rc = kvs_rbtree_mod(&t, key, newv);
        EXPECT_EQ_INT(rc, 0);
        EXPECT_EQ_STR(kvs_rbtree_get(&t, key), newv);
    }

    // 4) 删除 1/3 key
    for (int i = 0; i < N; i += 3) {
        char key[64];
        snprintf(key, sizeof(key), "key_%06d", i);

        int rc = kvs_rbtree_del(&t, key);
        EXPECT_EQ_INT(rc, 0);

        EXPECT_EQ_INT(kvs_rbtree_exist(&t, key), 1);
        EXPECT_TRUE(kvs_rbtree_get(&t, key) == NULL);
    }

    // 5) 校验剩余 key：存在且 value 正确（被 mod 的是 val2，否则 val）
    for (int i = 0; i < N; ++i) {
        char key[64];
        snprintf(key, sizeof(key), "key_%06d", i);

        if (i % 3 == 0) {
            EXPECT_EQ_INT(kvs_rbtree_exist(&t, key), 1);
            EXPECT_TRUE(kvs_rbtree_get(&t, key) == NULL);
            continue;
        }

        EXPECT_EQ_INT(kvs_rbtree_exist(&t, key), 0);

        char expect[64];
        if (i % 2 == 0) snprintf(expect, sizeof(expect), "val2_%06d", i);
        else           snprintf(expect, sizeof(expect), "val_%06d", i);

        EXPECT_EQ_STR(kvs_rbtree_get(&t, key), expect);
    }

    // 6) 删除不存在 key，返回 1
    EXPECT_EQ_INT(kvs_rbtree_del(&t, "this_key_not_exists"), 1);

    free(idx);
    kvs_rbtree_destory(&t);
}

int main(void) {
    printf("[TEST] rbtree: basic_api...\n");
    test_basic_api();
    printf("[PASS] basic_api\n");

    printf("[TEST] rbtree: mass_insert_modify_delete...\n");
    test_mass_insert_modify_delete();
    printf("[PASS] mass_insert_modify_delete\n");

    printf("[ALL PASS]\n");
    return 0;
}
