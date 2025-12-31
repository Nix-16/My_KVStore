#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator/kvs_alloc.h"

#define MAX_TABLE_SIZE 1024

typedef struct hashnode_s
{
    char *key;
    char *value;
    struct hashnode_s *next;

} hashnode_t;

typedef struct hashtable_s
{

    hashnode_t **nodes; //* change **,

    int max_slots;
    int count;

} hashtable_t;

typedef struct hashtable_s kvs_hash_t;
int kvs_hash_count(kvs_hash_t *hash);

// 5+2
int kvs_hash_create(kvs_hash_t *hash);
void kvs_hash_destory(kvs_hash_t *hash);

int kvs_hash_set(hashtable_t *hash, char *key, char *value);
char *kvs_hash_get(kvs_hash_t *hash, char *key);
int kvs_hash_mod(kvs_hash_t *hash, char *key, char *value);
int kvs_hash_del(kvs_hash_t *hash, char *key);
int kvs_hash_exist(kvs_hash_t *hash, char *key);