#include "engine/kvs_hash.h"

kvs_hash_t global_hash;

static int _hash(char *key, int size)
{

    if (!key)
        return -1;

    int sum = 0;
    int i = 0;

    while (key[i] != 0)
    {
        sum += key[i];
        i++;
    }

    return sum % size;
}

static hashnode_t *_create_node(const char *key, const char *value)
{
    hashnode_t *node = (hashnode_t *)kvs_malloc(sizeof(*node));
    if (!node)
        return NULL;

    size_t klen = strlen(key);
    node->key = (char *)kvs_malloc(klen + 1);
    if (!node->key)
    {
        kvs_free(node);
        return NULL;
    }
    memcpy(node->key, key, klen + 1);

    size_t vlen = strlen(value);
    node->value = (char *)kvs_malloc(vlen + 1);
    if (!node->value)
    {
        kvs_free(node->key);
        kvs_free(node);
        return NULL;
    }
    memcpy(node->value, value, vlen + 1);

    node->next = NULL;
    return node;
}

//
int kvs_hash_create(kvs_hash_t *hash)
{

    if (!hash)
        return -1;

    if (hash->nodes)
        return -1;

    hash->nodes = (hashnode_t **)kvs_malloc(sizeof(hashnode_t *) * MAX_TABLE_SIZE);
    if (!hash->nodes)
        return -1;
    memset(hash->nodes, 0, sizeof(hashnode_t *) * MAX_TABLE_SIZE);

    hash->max_slots = MAX_TABLE_SIZE;
    hash->count = 0;

    return 0;
}

//
void kvs_hash_destory(kvs_hash_t *hash)
{
    if (!hash)
        return;

    for (int i = 0; i < hash->max_slots; i++)
    {
        hashnode_t *node = hash->nodes[i];
        while (node)
        {
            hashnode_t *next = node->next;
            kvs_free(node->key);
            kvs_free(node->value);
            kvs_free(node);
            node = next;
        }
        hash->nodes[i] = NULL;
    }

    kvs_free(hash->nodes);
    hash->nodes = NULL;
    hash->max_slots = 0;
    hash->count = 0;
}

// mp
int kvs_hash_set(kvs_hash_t *hash, char *key, char *value)
{

    if (!hash || !key || !value)
        return -1;

    int idx = _hash(key, hash->max_slots);

    hashnode_t *node = hash->nodes[idx];

    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        { // exist
            return 1;
        }
        node = node->next;
    }

    hashnode_t *new_node = _create_node(key, value);
    if (!new_node)
        return -2;
    new_node->next = hash->nodes[idx];
    hash->nodes[idx] = new_node;

    hash->count++;

    return 0;
}

char *kvs_hash_get(kvs_hash_t *hash, char *key)
{

    if (!hash || !key)
        return NULL;

    int idx = _hash(key, hash->max_slots);

    hashnode_t *node = hash->nodes[idx];

    while (node != NULL)
    {

        if (strcmp(node->key, key) == 0)
        {
            return node->value;
        }

        node = node->next;
    }

    return NULL;
}

int kvs_hash_mod(kvs_hash_t *hash, char *key, char *value)
{
    if (!hash || !key || !value)
        return -1;

    int idx = _hash(key, hash->max_slots);
    hashnode_t *node = hash->nodes[idx];
    while (node && strcmp(node->key, key) != 0)
        node = node->next;
    if (!node)
        return 1;

    size_t vlen = strlen(value);
    char *newv = kvs_malloc(vlen + 1);
    if (!newv)
        return -2;
    memcpy(newv, value, vlen + 1);

    kvs_free(node->value);
    node->value = newv;
    return 0;
}

int kvs_hash_count(kvs_hash_t *hash)
{
    return hash ? hash->count : 0;
}

int kvs_hash_del(kvs_hash_t *hash, char *key)
{
    if (!hash || !key)
        return -1;

    int idx = _hash(key, hash->max_slots);

    hashnode_t *head = hash->nodes[idx];
    if (head == NULL)
        return 1; // noexist
    // head node
    if (strcmp(head->key, key) == 0)
    {
        hashnode_t *tmp = head->next;
        hash->nodes[idx] = tmp;

        kvs_free(head->key);
        kvs_free(head->value);
        kvs_free(head);

        hash->count--;

        return 0;
    }

    hashnode_t *cur = head;
    while (cur->next != NULL)
    {
        if (strcmp(cur->next->key, key) == 0)
            break; // search node

        cur = cur->next;
    }

    if (cur->next == NULL)
    {

        return 1;
    }

    hashnode_t *tmp = cur->next;
    cur->next = tmp->next;
    kvs_free(tmp->key);
    kvs_free(tmp->value);

    kvs_free(tmp);

    hash->count--;

    return 0;
}

int kvs_hash_exist(kvs_hash_t *hash, char *key)
{
    if (!hash || !key)
        return -1;
    int idx = _hash(key, hash->max_slots);
    for (hashnode_t *n = hash->nodes[idx]; n; n = n->next)
        if (strcmp(n->key, key) == 0)
            return 0;
    return 1;
}
