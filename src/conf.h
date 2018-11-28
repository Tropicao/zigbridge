#ifndef ZG_CONF_H
#define ZG_CONF_H

#include <stdint.h>

uint8_t zg_conf_init(char *conf_path);
void zg_conf_shutdown();
const char *zg_conf_get_znp_device_path();
int zg_conf_get_znp_baudrate();
const char *zg_conf_get_network_key_path();
const char *zg_conf_get_device_list_path();
const char *zg_conf_get_http_server_address();
int zg_conf_get_http_server_port();
const char *zg_conf_get_tcp_server_address();
int zg_conf_get_tcp_server_port();

#endif

