#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "engine/kvs_hash.h"

#define EXPECT_TRUE(x) do { \
    if (!(x)) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_TRUE(%s)\n", __FILE__, __LINE__, #x); \
        assert(x); \
    } \
} while (0)

#define EXPECT_EQ_INT(a,b) do { \
    int _va = (a); \
    int _vb = (b); \
    if (_va != _vb) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_EQ_INT(%s=%d, %s=%d)\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        assert(_va == _vb); \
    } \
} while (0)

#define EXPECT_STREQ(a,b) do { \
    const char *_sa = (a); \
    const char *_sb = (b); \
    if (!_sa || !_sb || strcmp(_sa, _sb) != 0) { \
        fprintf(stderr, "[FAIL] %s:%d: EXPECT_STREQ(%s=\"%s\", %s=\"%s\")\n", \
                __FILE__, __LINE__, #a, _sa ? _sa : "(null)", #b, _sb ? _sb : "(null)"); \
        assert(_sa && _sb && strcmp(_sa, _sb) == 0); \
    } \
} while (0)

static void test_basic_api(void)
{
    printf("[TEST] hash: basic_api...\n");

    kvs_hash_t h = {0};

    EXPECT_EQ_INT(kvs_hash_create(&h), 0);
    EXPECT_TRUE(h.nodes != NULL);
    EXPECT_EQ_INT(h.count, 0);
    EXPECT_EQ_INT(h.max_slots, MAX_TABLE_SIZE);

    // set/get
    EXPECT_EQ_INT(kvs_hash_set(&h, "k1", "v1"), 0);
    EXPECT_EQ_INT(kvs_hash_set(&h, "k2", "v2"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 2);

    EXPECT_STREQ(kvs_hash_get(&h, "k1"), "v1");
    EXPECT_STREQ(kvs_hash_get(&h, "k2"), "v2");
    EXPECT_TRUE(kvs_hash_get(&h, "k3") == NULL);

    // exist
    EXPECT_EQ_INT(kvs_hash_exist(&h, "k1"), 0);
    EXPECT_EQ_INT(kvs_hash_exist(&h, "k3"), 1);

    // duplicate set
    EXPECT_EQ_INT(kvs_hash_set(&h, "k1", "v1_new"), 1);   // already exists
    EXPECT_EQ_INT(kvs_hash_count(&h), 2);
    EXPECT_STREQ(kvs_hash_get(&h, "k1"), "v1");          // 仍是旧值（set 不覆盖）

    // mod
    EXPECT_EQ_INT(kvs_hash_mod(&h, "k1", "v1_new"), 0);
    EXPECT_STREQ(kvs_hash_get(&h, "k1"), "v1_new");
    EXPECT_EQ_INT(kvs_hash_mod(&h, "k_not_exist", "x"), 1);

    // del
    EXPECT_EQ_INT(kvs_hash_del(&h, "k2"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 1);
    EXPECT_TRUE(kvs_hash_get(&h, "k2") == NULL);
    EXPECT_EQ_INT(kvs_hash_del(&h, "k2"), 1); // noexist

    kvs_hash_destory(&h);
    EXPECT_TRUE(h.nodes == NULL);
    EXPECT_EQ_INT(h.count, 0);
    EXPECT_EQ_INT(h.max_slots, 0);
}

static void test_collision_and_delete_positions(void)
{
    printf("[TEST] hash: collision_delete_positions...\n");

    kvs_hash_t h = {0};
    EXPECT_EQ_INT(kvs_hash_create(&h), 0);

    // 这三个 key 在 sum-hash 下总和相同 -> 必然同桶：
    // "abc"(97+98+99) == "acb"(97+99+98) == "bac"(98+97+99)
    EXPECT_EQ_INT(kvs_hash_set(&h, "abc", "v_abc"), 0);
    EXPECT_EQ_INT(kvs_hash_set(&h, "acb", "v_acb"), 0);
    EXPECT_EQ_INT(kvs_hash_set(&h, "bac", "v_bac"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 3);

    EXPECT_STREQ(kvs_hash_get(&h, "abc"), "v_abc");
    EXPECT_STREQ(kvs_hash_get(&h, "acb"), "v_acb");
    EXPECT_STREQ(kvs_hash_get(&h, "bac"), "v_bac");

    // 删除 middle（取决于插入顺序，你是头插：最后插入的是 head）
    // 插入顺序 abc -> acb -> bac，因此链表：bac(head) -> acb(middle) -> abc(tail)

    // delete middle
    EXPECT_EQ_INT(kvs_hash_del(&h, "acb"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 2);
    EXPECT_TRUE(kvs_hash_get(&h, "acb") == NULL);
    EXPECT_STREQ(kvs_hash_get(&h, "abc"), "v_abc");
    EXPECT_STREQ(kvs_hash_get(&h, "bac"), "v_bac");

    // delete tail
    EXPECT_EQ_INT(kvs_hash_del(&h, "abc"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 1);
    EXPECT_TRUE(kvs_hash_get(&h, "abc") == NULL);
    EXPECT_STREQ(kvs_hash_get(&h, "bac"), "v_bac");

    // delete head
    EXPECT_EQ_INT(kvs_hash_del(&h, "bac"), 0);
    EXPECT_EQ_INT(kvs_hash_count(&h), 0);
    EXPECT_TRUE(kvs_hash_get(&h, "bac") == NULL);

    kvs_hash_destory(&h);
}

static void test_invalid_args(void)
{
    printf("[TEST] hash: invalid_args...\n");

    kvs_hash_t h = {0};

    EXPECT_EQ_INT(kvs_hash_create(NULL), -1);

    EXPECT_EQ_INT(kvs_hash_set(NULL, "k", "v"), -1);
    EXPECT_EQ_INT(kvs_hash_mod(NULL, "k", "v"), -1);
    EXPECT_EQ_INT(kvs_hash_del(NULL, "k"), -1);
    EXPECT_EQ_INT(kvs_hash_exist(NULL, "k"), -1);
    EXPECT_TRUE(kvs_hash_get(NULL, "k") == NULL);
    EXPECT_EQ_INT(kvs_hash_count(NULL), 0);

    EXPECT_EQ_INT(kvs_hash_create(&h), 0);

    EXPECT_EQ_INT(kvs_hash_set(&h, NULL, "v"), -1);
    EXPECT_EQ_INT(kvs_hash_set(&h, "k", NULL), -1);
    EXPECT_TRUE(kvs_hash_get(&h, NULL) == NULL);
    EXPECT_EQ_INT(kvs_hash_mod(&h, NULL, "v"), -1);
    EXPECT_EQ_INT(kvs_hash_mod(&h, "k", NULL), -1);
    EXPECT_EQ_INT(kvs_hash_del(&h, NULL), -1);
    EXPECT_EQ_INT(kvs_hash_exist(&h, NULL), -1);

    kvs_hash_destory(&h);
}

int main(void)
{
    test_basic_api();
    test_collision_and_delete_positions();
    test_invalid_args();

    printf("[OK] all kvs_hash unit tests passed.\n");
    return 0;
}
