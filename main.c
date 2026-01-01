#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/config/kvs_config.h"
#include "include/allocator/kvs_alloc.h"   

int main()
{
    kvs_config_t config;

    kvs_config_load_file(&config, "conf/kvs.conf");
    printf("config: bind_ip=%s, port=%d, allocator=%d, network=%d\n",
           config.bind_ip, config.port, config.allocator, config.network);
   kvs_set_allocator(config.allocator);
    return 0;
}