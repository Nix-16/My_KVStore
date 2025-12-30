#include "engine/kvs_array.h"

// singleton

kvs_array_t global_array = {0};

int kvs_array_create(kvs_array_t *inst)
{

    if (!inst)
        return -1;
    if (inst->table)
    {
        printf("table has alloc\n");
        return -1;
    }
    inst->table = kvs_malloc(KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));
    if (!inst->table)
    {
        return -1;
    }

    memset(inst->table, 0, KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));

    inst->total = 0;
    inst->idx = 0;

    return 0;
}

void kvs_array_destory(kvs_array_t *inst)
{

    if (!inst)
        return;

    if (inst->table)
    {

        /* 释放每个 item 的 key/value，避免内存泄漏 */
        int i = 0;
        for (i = 0; i < inst->total; i++)
        {

            if (inst->table[i].key)
            {
                kvs_free(inst->table[i].key);
                inst->table[i].key = NULL;
            }

            if (inst->table[i].value)
            {
                kvs_free(inst->table[i].value);
                inst->table[i].value = NULL;
            }
        }

        kvs_free(inst->table);
        inst->table = NULL;
    }

    inst->total = 0;
    inst->idx = 0;
}

/*
 * @return: <0, error; =0, success; >0, exist
 */

int kvs_array_set(kvs_array_t *inst, char *key, char *value)
{

    if (inst == NULL || inst->table == NULL || key == NULL || value == NULL)
        return -1;

    char *str = kvs_array_get(inst, key);
    if (str)
    {
        return 1; //
    }

    char *kcopy = kvs_malloc(strlen(key) + 1);
    if (kcopy == NULL)
        return -2;
    memset(kcopy, 0, strlen(key) + 1);
    strncpy(kcopy, key, strlen(key));

    char *kvalue = kvs_malloc(strlen(value) + 1);
    if (kvalue == NULL)
    {
        kvs_free(kcopy);
        return -2;
    }

    memset(kvalue, 0, strlen(value) + 1);
    strncpy(kvalue, value, strlen(value));

    /* 1) 先找空洞填补：注意填补空洞时 total 不应 ++ */
    int i = 0;
    for (i = 0; i < inst->total; i++)
    {
        if (inst->table[i].key == NULL)
        {

            inst->table[i].key = kcopy;
            inst->table[i].value = kvalue;

            return 0;
        }
    }

    /* 2) 没空洞：追加到末尾；这里才需要 total++ */
    if (inst->total >= KVS_ARRAY_SIZE)
    {
        /* 修复：避免越界；并释放刚申请的内存 */
        kvs_free(kcopy);
        kvs_free(kvalue);
        return -1;
    }

    inst->table[inst->total].key = kcopy;
    inst->table[inst->total].value = kvalue;
    inst->total++;

    return 0;
}

char *kvs_array_get(kvs_array_t *inst, char *key)
{

    if (inst == NULL || inst->table == NULL || key == NULL)
        return NULL;

    int i = 0;
    for (i = 0; i < inst->total; i++)
    {
        if (inst->table[i].key == NULL)
        {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0)
        {
            return inst->table[i].value;
        }
    }

    return NULL;
}

/*
 * @return < 0, error;  =0,  success; >0, no exist
 */

int kvs_array_del(kvs_array_t *inst, char *key)
{

    if (inst == NULL || inst->table == NULL || key == NULL)
        return -1;

    int i = 0;
    for (i = 0; i < inst->total; i++)
    {
        /* 修复：空洞情况下避免 strcmp(NULL, key) */
        if (inst->table[i].key == NULL)
        {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0)
        {

            kvs_free(inst->table[i].key);
            inst->table[i].key = NULL;

            kvs_free(inst->table[i].value);
            inst->table[i].value = NULL;

            /*
             * 修复：删除后回收尾部空洞，避免 total 长期不减导致“>1024/逻辑越界”
             * 保持“空洞数组”语义：不做紧凑回填，只回收末尾连续空洞
             */
            while (inst->total > 0 && inst->table[inst->total - 1].key == NULL)
            {
                inst->total--;
            }

            return 0;
        }
    }

    return 1; 
}

/*
 * @return : < 0, error; =0, success; >0, no exist
 */

int kvs_array_mod(kvs_array_t *inst, char *key, char *value)
{

    if (inst == NULL || inst->table == NULL || key == NULL || value == NULL)
        return -1;
    // error: > 1024
    if (inst->total == 0)
    {
        return 1;
    }

    int i = 0;
    for (i = 0; i < inst->total; i++)
    {

        if (inst->table[i].key == NULL)
        {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0)
        {

            char *kvalue = kvs_malloc(strlen(value) + 1);
            if (kvalue == NULL)
                return -2;
            memset(kvalue, 0, strlen(value) + 1);
            strncpy(kvalue, value, strlen(value));

            kvs_free(inst->table[i].value); // 释放旧值
            inst->table[i].value = kvalue;

            return 0;
        }
    }

    return 1; 
}

/*
 * @return 0: exist, 1: no exist
 */
int kvs_array_exist(kvs_array_t *inst, char *key)
{
    if (!inst || !inst->table || !key)
        return -1; // error

    int i = 0;
    for (i = 0; i < inst->total; i++)
    {
        if (inst->table[i].key == NULL)
            continue;
        if (strcmp(inst->table[i].key, key) == 0)
            return 0; // exist
    }
    return 1; // no exist
}
