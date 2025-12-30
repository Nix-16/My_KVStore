#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "engine/kvs_array.h"
#include "allocator/kvs_alloc.h"

/*
 * 说明：
 * - 使用 assert 做断言，任何失败会直接中止并打印行号
 * - 测试直接链接真实 src/allocator/kvs_alloc.c（malloc/free 封装）
 * - 假设 kvs_array_set 会拷贝 key/value（你的实现确实是这样）
 */

static void test_create_destroy(void)
{
    kvs_array_t a = {0};

    int rc = kvs_array_create(&a);
    assert(rc == 0);
    assert(a.table != NULL);
    assert(a.total == 0);

    kvs_array_destory(&a);
    assert(a.table == NULL);
    assert(a.total == 0);

    /* destroy 后可以再次 create */
    rc = kvs_array_create(&a);
    assert(rc == 0);
    assert(a.table != NULL);

    kvs_array_destory(&a);
    assert(a.table == NULL);
}

static void test_set_get_exist_duplicate(void)
{
    kvs_array_t a = {0};
    assert(kvs_array_create(&a) == 0);

    assert(kvs_array_exist(&a, "k1") == 1);
    assert(kvs_array_get(&a, "k1") == NULL);

    assert(kvs_array_set(&a, "k1", "v1") == 0);
    assert(kvs_array_exist(&a, "k1") == 0);

    char *v = kvs_array_get(&a, "k1");
    assert(v != NULL);
    assert(strcmp(v, "v1") == 0);

    /* 重复 key：按你语义返回 1（不覆盖） */
    assert(kvs_array_set(&a, "k1", "v1_new") == 1);
    v = kvs_array_get(&a, "k1");
    assert(strcmp(v, "v1") == 0);

    kvs_array_destory(&a);
}

static void test_mod_del_basic(void)
{
    kvs_array_t a = {0};
    assert(kvs_array_create(&a) == 0);

    assert(kvs_array_set(&a, "k1", "v1") == 0);

    /* mod 成功 */
    assert(kvs_array_mod(&a, "k1", "v1_mod") == 0);
    char *v = kvs_array_get(&a, "k1");
    assert(v != NULL);
    assert(strcmp(v, "v1_mod") == 0);

    /* mod 不存在：你实现可能返回 1 或 i(=total)，这里用 >0 兼容 */
    int rc = kvs_array_mod(&a, "k_not_exist", "x");
    assert(rc > 0);

    /* del 成功 */
    assert(kvs_array_del(&a, "k1") == 0);
    assert(kvs_array_get(&a, "k1") == NULL);

    /* del 不存在：同样用 >0 兼容 */
    rc = kvs_array_del(&a, "k1");
    assert(rc > 0);

    kvs_array_destory(&a);
}

static void test_capacity_limit_1024_1025(void)
{
    kvs_array_t a = {0};
    assert(kvs_array_create(&a) == 0);

    char keybuf[64];
    char valbuf[64];

    /* 插满 1024 个不同 key */
    for (int i = 0; i < KVS_ARRAY_SIZE; i++)
    {
        snprintf(keybuf, sizeof(keybuf), "k_%d", i);
        snprintf(valbuf, sizeof(valbuf), "v_%d", i);
        int rc = kvs_array_set(&a, keybuf, valbuf);
        assert(rc == 0);
    }

    /* 第 1025 个应失败：返回 -1（容量满）或 -2（极端：内存不足） */
    int rc = kvs_array_set(&a, "k_over", "v_over");
    assert(rc < 0);

    kvs_array_destory(&a);
}

static void test_hole_reuse_when_full(void)
{
    kvs_array_t a = {0};
    assert(kvs_array_create(&a) == 0);

    char keybuf[64];
    char valbuf[64];

    /* 插满 */
    for (int i = 0; i < KVS_ARRAY_SIZE; i++)
    {
        snprintf(keybuf, sizeof(keybuf), "k_%d", i);
        snprintf(valbuf, sizeof(valbuf), "v_%d", i);
        assert(kvs_array_set(&a, keybuf, valbuf) == 0);
    }

    /* 删除中间 10 个 key，制造空洞（total 很可能仍是 1024） */
    for (int i = 100; i < 110; i++)
    {
        snprintf(keybuf, sizeof(keybuf), "k_%d", i);
        assert(kvs_array_del(&a, keybuf) == 0);
        assert(kvs_array_get(&a, keybuf) == NULL);
    }

    /*
     * 关键验证：即便 total == 1024，只要内部有空洞，仍应能继续 set 成功（填洞）
     */
    for (int i = 0; i < 10; i++)
    {
        snprintf(keybuf, sizeof(keybuf), "k_new_%d", i);
        snprintf(valbuf, sizeof(valbuf), "v_new_%d", i);
        int rc = kvs_array_set(&a, keybuf, valbuf);
        assert(rc == 0);
        assert(kvs_array_get(&a, keybuf) != NULL);
    }

    /* 再插入一个：可能成功也可能失败（取决于洞是否刚好填完），但至少不能崩溃 */
    (void)kvs_array_set(&a, "k_new_over", "v_new_over");

    kvs_array_destory(&a);
}

int main(void)
{
    test_create_destroy();
    test_set_get_exist_duplicate();
    test_mod_del_basic();
    test_capacity_limit_1024_1025();
    test_hole_reuse_when_full();

    printf("[OK] all kvs_array unit tests passed.\n");
    return 0;
}
