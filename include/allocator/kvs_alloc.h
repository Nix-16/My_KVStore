#pragma once
#include <stddef.h>
#include <stdlib.h>

void *kvs_malloc(size_t size);

void kvs_free(void *ptr);
