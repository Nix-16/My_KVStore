#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator/kvs_alloc.h"

#define KVS_ARRAY_SIZE 1024

typedef struct kvs_array_item_s
{
    char *key;
    char *value;
} kvs_array_item_t;

typedef struct kvs_array_s
{
    kvs_array_item_t *table;
    int idx;   // 目前实现未使用，可考虑删除或用于优化插入位置
    int total; // 当前有效元素数量
} kvs_array_t;

// 5+2

int kvs_array_create(kvs_array_t *inst);
void kvs_array_destory(kvs_array_t *inst);

int kvs_array_set(kvs_array_t *inst, char *key, char *value);
char *kvs_array_get(kvs_array_t *inst, char *key);
int kvs_array_del(kvs_array_t *inst, char *key);
int kvs_array_mod(kvs_array_t *inst, char *key, char *value);
int kvs_array_exist(kvs_array_t *inst, char *key);
