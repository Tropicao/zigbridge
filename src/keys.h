#ifndef ZG_KEYS_H
#define ZG_KEYS_H

#include <stdint.h>

/**
 * \brief Initialize any security key used by application
 *
 * This function will try to load keys stored on host (e.g. : network key)
 */
void zg_keys_init();

/**
 * \brief Deinitialize loaded keys
 */
void zg_keys_shutdown();

/**
 * \brief Return the network key, previously loaded by zg_keys_init
 * \return A pointer on the table containing the network key if key has been
 * loaded, otherwise NULL
 */
uint8_t *zg_keys_network_key_get();

/**
 * \brief Get the size of the network key
 *  This function main purpose is ti be used with zg_keys_network_key_get
 * \return The network key size
 */
uint8_t zg_keys_network_key_size_get();

/**
 * \brief Delete the currently used network key
 *
 * An usage example of this API can be to reset the gateway network and start a
 * new network (i.e. with a new network key)
 */
void zg_keys_network_key_del(void);

/**
 * \brief Indicates if a network key has already been created and saved on host
 * \return 1 if key exists, otherwise 0
 */
uint8_t zg_keys_check_network_key_exists(void);

#endif

