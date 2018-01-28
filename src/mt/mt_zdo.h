#ifndef ZG_MT_ZDO_H
#define ZG_MT_ZDO_H

#include <stdint.h>
#include "types.h"


/**
 * \brief Initialize the MT ZDO module
 *
 * This function registers mandatory callbacks to RPC system to receive MT ZDO
 * messages
 * \return 0 if initialization has passed properly, otherwise 1
 */
int zg_mt_zdo_init(void);

/**
 * \brief Terminate the MT ZDO module
 */
void zg_mt_zdo_shutdown(void);

/**
 * \brief Ask ZNP to start the network stack
 * This API must be used after processing all needed configuration (addressing,
 * security, etc)
 * \param cb The callback triggered when ZNP has received and processed the
 * command
 */
void zg_mt_zdo_startup_from_app(SyncActionCb);

/**
 * \brief Register a callback to be called when a device issue a DEVICE_ANNCE
 * message
 * \param cb The callback to be called on new DEVICE_ANNCE message.
 */
void zg_mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr));

/**
 * \brief Ask ZNP to send a DEVICE_ANNCE message on network, to announce gateway
 * availability to all devices
 * \param cb The callback triggered when ZNP has received and processed the
 * command
 */
void zg_mt_zdo_device_annce(uint16_t addr, uint64_t uid, SyncActionCb cb);


/**
 * \brief Ask ZNP to send a route request message on network, to search for a
 * specific device
 * \param addr The short address of the device we want to find on network
 * \param cb The callback triggered when ZNP has received and processed the
 * command
 */
void zg_mt_zdo_ext_route_disc_request(uint16_t addr, SyncActionCb cb);

/**
 * \brief Ask ZNP to send an ACTIVE ENDPOINTS REQUEST message on network, to get
 * the list of available applications on a distant device. This function must be
 * used in the general process of discovering and learning a new device
 * If successfull, this command should make the remote device generate an ACTIVE
 * ENDPOINTS response, containing the list of its endpoints
 * \param short_addr The short address of the device which we want to learn the
 * active endpoints
 * \param cb The callback triggered when ZNP has received and processed the
 * command
 */
void zg_mt_zdo_query_active_endpoints(uint16_t short_addr, SyncActionCb cb);

/**
 * \brief Register a callback on active endpoints response
 * This function allow a higher application level to get acknowledged when such
 * a response arrives
 * \param cb The callback which will be called when a new device sends its
 * endpoint list
 */
void zg_mt_zdo_register_active_ep_rsp_callback(void (*cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list));

/**
 * \brief Register a callback on simple descriptor response
 * This function allow a higher application level to get acknowledged when such
 * a response arrives
 * \param cb The callback which will be called when a new device sends a
 * description of a queried endpoint
 */
void zg_mt_zdo_register_simple_desc_rsp_cb(void (*cb)(uint8_t endpoint, uint16_t profile, uint16_t device_id));

/**
 * \brief Open the network to allow new devices to join
 * This function will open the network for a default duration of 32s
 * \param cb The callback triggered when ZNP has received and processed the
 * command
 */
void zg_mt_zdo_permit_join(SyncActionCb cb);

#endif

