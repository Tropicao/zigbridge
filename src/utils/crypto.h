#ifndef ZG_CRYPTO_H
#define ZG_CRYPTO_H

#include <stdint.h>

int zg_crypto_init(void);
void zg_crypto_shutdown(void);
uint8_t zg_crypto_encrypt_nwk_key(uint8_t *key);

#endif

