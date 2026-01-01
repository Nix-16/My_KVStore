#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "config/kvs_config.h" 

struct mp_large_s
{
    struct mp_large_s *next;
    void *alloc;
};

struct mp_node_s
{
    unsigned char *last; // 当前可分配起始地址
    unsigned char *end;  // 本node结束地址
    struct mp_node_s *next;
    size_t failed; // node分配失败次数
};

struct mp_pool_s
{
    size_t max; // 小块分配上限，超过走large

    struct mp_node_s *current; // 当前node
    struct mp_large_s *large;  // 大块链表

    struct mp_node_s head[0];
};

struct mp_pool_s *mp_create_pool(size_t size);
void mp_destory_pool(struct mp_pool_s *pool);
void *mp_alloc(struct mp_pool_s *pool, size_t size);  // 对齐，不清零
void *mp_nalloc(struct mp_pool_s *pool, size_t size); // 不对齐，不清零
void *mp_calloc(struct mp_pool_s *pool, size_t size); // 对齐，清零
void mp_free(struct mp_pool_s *pool, void *p);

void kvs_set_allocator(kvs_alloc_type_t type);

void *kvs_malloc(size_t size);

void kvs_free(void *ptr);
