#include "allocator/kvs_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

// 默认：system malloc/free
static void *(*g_malloc_fn)(size_t) = malloc;
static void (*g_free_fn)(void *) = free;

#define MP_ALIGNMENT 32
#define MP_PAGE_SIZE 4096
#define MP_MAX_ALLOC_FROM_POOL (MP_PAGE_SIZE - 1)

#define mp_align(n, alignment) (((n) + (alignment - 1)) & ~(alignment - 1))
#define mp_align_ptr(p, alignment) (void *)((((size_t)p) + (alignment - 1)) & ~(alignment - 1))

struct mp_pool_s *global_pool = NULL;

static void *mypool_malloc_wrap(size_t size)
{
    if (!global_pool)
    {
        global_pool = mp_create_pool(MP_PAGE_SIZE);
        if (!global_pool)
            return NULL;
    }
    return mp_nalloc(global_pool, size);
}

static void mypool_free_wrap(void *ptr)
{
    if (!ptr || !global_pool)
        return;
    mp_free(global_pool, ptr);
}

struct mp_pool_s *mp_create_pool(size_t size)
{
    struct mp_pool_s *p;

    int ret = posix_memalign((void **)&p, MP_ALIGNMENT, size + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s));
    if (ret)
    {
        return NULL;
    }

    p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
    p->current = p->head;
    p->large = NULL;

    p->head->last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
    p->head->end = p->head->last + size;
    p->head->failed = 0;
    p->head->next = NULL;

    return p;
}

void mp_destory_pool(struct mp_pool_s *pool)
{
    struct mp_node_s *h, *n;
    struct mp_large_s *l;

    for (l = pool->large; l; l = l->next)
    {
        if (l->alloc)
        {
            free(l->alloc);
        }
    }

    h = pool->head->next;

    while (h)
    {
        n = h->next;
        free(h);
        h = n;
    }

    free(pool);
}

void mp_reset_pool(struct mp_pool_s *pool)
{
    struct mp_large_s *l;
    struct mp_node_s *h;

    for (l = pool->large; l; l = l->next)
    {
        if (l->alloc)
        {
            free(l->alloc);
        }
    }
    pool->large = NULL;

    for (h = pool->head; h; h = h->next)
    {
        h->last = (unsigned char *)h + sizeof(struct mp_node_s);
        h->failed = 0;
    }
    pool->current = pool->head;
}

static void *mp_alloc_large(struct mp_pool_s *pool, size_t size)
{
    void *p = malloc(size);
    if (p == NULL)
        return NULL;

    size_t n = 0;
    struct mp_large_s *large;
    for (large = pool->large; large; large = large->next)
    {
        if (large->alloc == NULL)
        {
            large->alloc = p;
            return p;
        }

        if (n++ > 3)
        {
            break;
        }
    }

    large = mp_alloc(pool, sizeof(struct mp_large_s));
    if (large == NULL)
    {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

static void *mp_alloc_block(struct mp_pool_s *pool, size_t size)
{

    unsigned char *m;
    struct mp_node_s *h = pool->head;
    size_t psize = (size_t)(h->end - (unsigned char *)h);

    // 防止 size 比一个 block 还大（照你现在的使用场景，一般不会发生）
    if (size + MP_ALIGNMENT + sizeof(struct mp_node_s) > psize)
    {
        return NULL;
    }

    int ret = posix_memalign((void **)&m, MP_ALIGNMENT, psize);
    if (ret)
        return NULL;

    struct mp_node_s *p, *new_node, *current;
    new_node = (struct mp_node_s *)m;

    new_node->end = m + psize;
    new_node->next = NULL;
    new_node->failed = 0;

    m += sizeof(struct mp_node_s);
    m = mp_align_ptr(m, MP_ALIGNMENT);
    new_node->last = m + size;

    current = pool->current;

    for (p = current; p->next; p = p->next)
    {
        if (p->failed++ > 4)
        { //
            current = p->next;
        }
    }
    p->next = new_node;

    pool->current = current ? current : new_node;

    return m;
}

void *mp_alloc(struct mp_pool_s *pool, size_t size)
{
    unsigned char *m;
    struct mp_node_s *p;

    if (size <= pool->max)
    {
        p = pool->current;

        do
        {
            m = mp_align_ptr(p->last, MP_ALIGNMENT);
            if ((size_t)(p->end - m) >= size)
            {
                p->last = m + size;
                return m;
            }
            p = p->next;
        } while (p);

        return mp_alloc_block(pool, size);
    }

    return mp_alloc_large(pool, size);
}

void *mp_nalloc(struct mp_pool_s *pool, size_t size)
{
    unsigned char *m;
    struct mp_node_s *p;

    if (size <= pool->max)
    {
        p = pool->current;

        do
        {
            m = p->last;
            if ((size_t)(p->end - m) >= size)
            {
                p->last = m + size;
                return m;
            }
            p = p->next;

        } while (p);

        return mp_alloc_block(pool, size);
    }

    return mp_alloc_large(pool, size);
}

void *mp_calloc(struct mp_pool_s *pool, size_t size)
{
    void *p = mp_alloc(pool, size);

    if (p)
    {
        memset(p, 0, size);
    }
    return p;
}

void mp_free(struct mp_pool_s *pool, void *p)
{
    struct mp_large_s *l;
    for (l = pool->large; l; l = l->next)
    {
        if (p == l->alloc)
        {
            free(l->alloc);
            l->alloc = NULL;
            return;
        }
    }
}

#ifdef USE_JEMALLOC
static void *je_malloc_wrap(size_t size) { return je_malloc(size); }
static void je_free_wrap(void *ptr) { je_free(ptr); }
#endif

void kvs_set_allocator(kvs_alloc_type_t type)
{
    switch (type)
    {
    case KVS_ALLOC_SYSTEM:
        g_malloc_fn = malloc;
        g_free_fn = free;
        printf("Using system malloc/free\n");
        break;

    case KVS_ALLOC_JEMALLOC:
#ifdef USE_JEMALLOC
        g_malloc_fn = je_malloc_wrap;
        g_free_fn = je_free_wrap;
        printf("Using jemalloc malloc/free\n");
        break;
#else
        // 没启用 jemalloc 就降级 system
        g_malloc_fn = malloc;
        g_free_fn = free;
        printf("jemalloc requested but not compiled in, fallback to system malloc/free\n");
        break;
#endif
    case KVS_ALLOC_MYPOOL:
        g_malloc_fn = mypool_malloc_wrap;
        g_free_fn = mypool_free_wrap;
        printf("Using mypool malloc/free\n");
        break;
    default:
        // 默认 system
        g_malloc_fn = malloc;
        g_free_fn = free;
        printf("Unknown allocator type, fallback to system malloc/free\n");
    }
}

void *kvs_malloc(size_t size)
{
    return g_malloc_fn(size);
}

void kvs_free(void *ptr)
{
    if (ptr)
        g_free_fn(ptr);
}