#ifndef ZG_MT_H
#define ZG_MT_H

/**
 * \brief Initialize all MT modules
 * This function is a helper to initialize all MT modules at once, instead of
 * manually enabling each one
 * It also takes care of enabling RPC module, since it is needed by any MT
 * subsystem module
 * \return 0 if all intializations have passed properly, otherwise 1
 */
int zg_mt_init();

/**
 * \brief Terminate all MT modules
 * This function is a helper to shut down all MT modules at once, instead of
 * manually shutting down each one. It also shuts down the RPC module, which is
 * not needed anymore since all MT subsystems modules are down
 */
void zg_mt_shutdown();

/**
 * \brief Send a ping to ZNP, a corresponding SRSP is expected
 * This function has to be removed, it is only here for tests purpose while
 * redesigning the network stack
 */
void zg_mt_test_ping(void);

#endif

