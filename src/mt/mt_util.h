#ifndef ZG_MT_UTIL_H
#define ZG_MT_UTIL_H

#include "types.h"


/**
 * \brief Initialize the MT UTIL module
 *
 * This function registers mandatory callbacks to RPC system to receive MT UTIL
 * messages
 * \return 0 if initialization has passed properly, otherwise 1
 */
int zg_mt_util_init(void);

/**
 * \brief Terminate the MT SYS module
 */
void zg_mt_util_shutdown(void);

/**
 * \brief Subscribe to ZNP UTIL callbacks
 *
 * \param cb The callback to be triggered when ZNP has received and processed
 * this command
 */
void zg_mt_util_af_subscribe_cmd (SyncActionCb cb);

#endif
