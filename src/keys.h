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
 * \brief Return the ZLL master key, previously loaded by zg_keys_init
 * \return A pointer on the table containing the ZLL master key if key has been
 * loaded, otherwise NULL
 */
uint8_t *zg_keys_zll_master_key_get();

/**
 * \brief Return the ZHA master key, previously loaded by zg_keys_init
 * \return A pointer on the table containing the ZHA master key if key has been
 * loaded, otherwise NULL
 */
uint8_t *zg_keys_zha_master_key_get();

/**
 * \brief Get the size of keys
 *  This function main purpose is to be used with zg_keys_network_key_get
 * \return The network key size
 */
uint8_t zg_keys_size_get();

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

/**
 * \brief Return the encrypted version of network key, processed as stated in
 * Zigbee specification
 * \param transport_key The key which must be used to encrypt the network key
 * \param encrypted the variable in which to store the encrypted key
 * \return 0 if everything went well, otherwise 0
 */
uint8_t zg_keys_get_encrypted_network_key_for_zll(uint32_t transaction_id, uint32_t response_id, uint8_t *encrypted);

/**
 * \brief Function used to test network key encryption for a key exchange in ZLL
 * context. Please refer to "Zigbee Light Link Standard", section 9, Annexe A to
 * get an overview of the test
 */
uint8_t zg_keys_test_nwk_key_encryption_zll(uint8_t *encrypted);

#endif

