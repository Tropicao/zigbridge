#ifndef ZG_MT_SYS_H
#define ZG_MT_SYS_H

#include <stdint.h>
#include "types.h"

typedef enum
{
    STARTUP_CLEAR_NWK_FRAME_COUNTER =   0x01,
    STARTUP_CLEAR_STATE =               0x02,
    STARTUP_CLEAR_CONFIG =              0x04,
} MtSysStartupOptions;

/**
 * \brief Initialize the MT SYS module
 *
 * This function registers mandatory callbacks to RPC system to receive MT SYS
 * messages
 * \return 0 if initialization has passed properly, otherwise 1
 */
int zg_mt_sys_init(void);

/**
 * \brief Terminate the MT SYS module
 */
void zg_mt_sys_shutdown(void);

/**
 * \brief Ask to zNP to process a soft reset
 *
 * When back, the dongle should issue a syS_RESET_IND message
 * \param cb The callback to be called when command has been processed by ZNP
 * (before resetting)
 */
void zg_mt_sys_reset_dongle(SyncActionCb cb);

/**
 * \brief Ping the ZNP and get its capabilities
 *
 * When pinged, the ZNP should repsond immediately with its capabilities (i.e
 * the built-in subsystems)
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_ping(SyncActionCb cb);

/**
 * \brief Write flags to select ZNP reset behavior
 * \param options Options that can be or-concatenated to select reset behavior
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_startup_options(MtSysStartupOptions options, SyncActionCb cb);

/**
 * \brief Write persistent data to use ZNP as coordinator
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_coord_flag(SyncActionCb cb);

/**
 * \brief Write persistent flag to disable any security. This function will make
 * all the traffic sent in clear
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_disable_security(SyncActionCb cb);

/**
 * \brief Write persistent flag to enable basic security level. This function will make
 * all the traffic sent encrypted with symmetric network key
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_enable_security(SyncActionCb cb);

/**
 * \brief Write in persistent memory the PAN id to be used
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_set_pan_id(SyncActionCb cb);

/**
 * \brief Write in persistent memory the network key to be used for
 * encrypting/decrypting data on the network
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_nwk_key(SyncActionCb cb);

/**
 * \brief Query ZNP for its extended address
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_check_ext_addr(SyncActionCb cb);

/**
 * \brief Return the extended address of ZNP radio head. Please note that this
 * will return the correct extended address only if a previous call to
 * zg_mt_sys_get_ext_addr has been done
 * \return The ZNP extended address on 64 bits
 */
uint64_t zg_mt_sys_get_ext_addr(void);

/**
 * \brief Write channel to be used on next reset in persistent memory
 * Please note that this setting will only be taken in account after next reset
 * \param cb The callback to be called when command has been processed by ZNP
 */
void zg_mt_sys_nv_write_channel(uint8_t channel, SyncActionCb cb);

void zg_mt_sys_nv_write_ext_pan_id(SyncActionCb cb);
#endif

