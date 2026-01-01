#pragma once

typedef enum
{
    KVS_ALLOC_SYSTEM = 0,
    KVS_ALLOC_JEMALLOC,
    KVS_ALLOC_MYPOOL
} kvs_alloc_type_t;

typedef enum
{
    KVS_NET_REACTOR = 0,
    KVS_NET_PROACTOR,
    KVS_NET_NTYCO
} kvs_net_type_t;

typedef struct
{
    char bind_ip[64];
    int port;

    kvs_alloc_type_t allocator;
    kvs_net_type_t network;

} kvs_config_t;

int kvs_config_load_file(kvs_config_t *cfg, const char *path);