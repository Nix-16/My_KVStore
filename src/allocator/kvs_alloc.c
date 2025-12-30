#include "allocator/kvs_alloc.h"

void *kvs_malloc(size_t size)
{
    return malloc(size);
}

void kvs_free(void *ptr)
{
    free(ptr);
}