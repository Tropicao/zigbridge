#ifndef ZG_KEYS_H
#define ZG_KEYS_H

#include <stdint.h>

uint8_t *zg_keys_network_key_get();
uint8_t zg_keys_network_key_size_get();
void zg_keys_network_key_del(void);
uint8_t zg_keys_check_network_key_exists(void);

#endif

