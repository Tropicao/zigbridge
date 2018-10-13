#ifndef ZG_APS_H
#define ZG_APS_H

#include <stdint.h>
#include "types.h"
#include "mt_af.h"

/* APS callbacks, registered by proper applications */
typedef void (*ApsMsgCb)(uint64_t addr, uint16_t cluster, void *data, int len);

/**
 * \brief Intialize the APS layer.
 * \return 0 if layer is properly initialized, otherwise 1
 */
int zg_aps_init();

/**
 * \brief Terminate the APS layer
 */
void zg_aps_shutdown();

/**
 * \brief Register a new application endpoint. This API has to main functions :
 * \li registering the endpoint on ZNP to enable sending/receiving of targeted
 * messages
 * \li registering one callback per application to process incoming applicative
 * messages
 * \param endpoint The endpoint number on which we want to register the
 * application. Since endpoint 0 is necessarily occupied by Zigbee device
 * profile, calling the function with endpoint 0 will only be useful to register
 * the ZDP messages callback
 * \param profile The application profile id to register on targeted endpoint
 * \param device_id The device id to register, specific to the application
 * \param device_ver The applicative devcie version
 * \param in_clusters_num The number of input clusters to register on the
 * endpoint
 * \param in_clusters_list the list of input clusters to register on the
 * endpoint
 * \param in_clusters_num The number of output clusters to register on the
 * endpoint
 * \param in_clusters_list the list of output clusters to register on the
 * endpoint
 * \param msg_cb The callback to be called whenever a new APS message targeted
 * to our registered application/endpoint arrives
 * \param cb A callback to trigger when the new registration is complete
 */
void zg_aps_register_endpoint(  uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                ApsMsgCb msg_cb,
                                SyncActionCb cb);

/**
 * \brief Send data to a specific remote application
 *
 * This API is the main function to send data to a remote entity. This entity
 * has to be fully defined by its network address, its destination endpoint, the
 * targeted cluster, and finally the cluster specific command
 * \param dst_addr The destination address of the device on the network
 * \param dst_pan The PAN ID on which is the target device
 * \param src_endpoint The local endpoint emitting the data
 * \param dst_endpoint The target remote endpoint
 * \param cluster The targeted cluster on the remote endpoint
 * \param command The command we want to send to remote cluster
 * \param data The command payload we want to send
 * \param len The command payload len
 * \param cb A callback to be called when data has been sent to ZNP
 */
void zg_aps_send_data(  uint64_t dst_addr,
                        AddrMode addr_mode,
                        uint16_t dst_pan,
                        uint8_t src_endpoint,
                        uint8_t dst_endpoint,
                        uint16_t cluster,
                        uint8_t command,
                        void *data,
                        int len,
                        SyncActionCb cb);

#endif

