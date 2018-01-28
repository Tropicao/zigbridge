#ifndef ZG_MT_AF_H
#define ZG_MT_AF_H

#include <stdint.h>
#include "types.h"

typedef void (*AfIncomingMessageCb)(uint8_t endpoint, uint16_t cluster, void *data, int len);

/**
 * \brief Initialize the MT AF module
 *
 * This function registers mandatory callbacks to RPC system to receive MT AF
 * messages
 * \return 0 if initialization has passed properly, otherwise 1
 */
int zg_mt_af_init(void);

/**
 * \brief Terminate the MT AF module
 */
void zg_mt_af_shutdown(void);

/**
 * \brief Register a new endpoint for the gateway
 *
 * This API registers new endpoint on ZNP to allow host to send messages for specific application
 * \param endpoint The endpoint value to register, between 1 and 255. Please
 * remember that 0 is the Zigbee Device Profile endpoint and thus cannot be
 * registered with this API since it is alreaduy defined.
 * \param profile The ID of application profile to register
 * \param device_id The device_id to register. This device id will be used by
 * end devcies and routers to identify the gateway as a specific device type
 * \param device_ver The device version of the endpoint
 * \param in_clusters_num The numbers of input cluster to register for this endpoint
 * \param in_clusters_list The list of input clusters to register for this
 * endpoint
 * \param out_clusters_num The numbers of output cluster to register for this endpoint
 * \param out_clusters_list The list of output clusters to register for this
 * endpoint
 * \param cb The callback to trigger when ZNP has received and processed the
 * register command
 */
void mt_af_register_endpoint(   uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                SyncActionCb cb);

/**
 * \brief Set targeted endpoint as inter-pan endpoint
 * This API is useful to switch an endpoint to inter-pan mode, enabling
 * inter-pan broadcast messages. It is used, for example, for touchlink
 * procedure
 * \param endpoint The endpoint to set as inter-pan endpoint
 * \param cb The callback to trigger when ZNP has received and processed the
 * command
 */
void mt_af_set_inter_pan_endpoint(uint8_t endpoint, SyncActionCb cb);

/**
 * \brief Set targeted channel as inter-pan channel
 * This API is useful to switch a channel to inter-pan mode, enabling
 * inter-channel broadcast messages. It is used, for example, for touchlink
 * procedure
 * \param channel The endpoint to set as inter-pan channel (value between 11 and
 * 26)
 * \param cb The callback to trigger when ZNP has received and processed the
 * command
 */
void mt_af_set_inter_pan_channel(uint8_t channel, SyncActionCb cb);

/**
 * \brief Register the callback to which module will transmit incoming
 * applicative messages
 * \param cb The callback which will be called on any new Applicative message,
 * regardless of targeted application/endpoint. It means that the receiving
 * callback will have to process and dispatch the message to proper application
 * layer
 */
void mt_af_register_incoming_message_callback(AfIncomingMessageCb cb);

/**
 * \brief Send an extended data request to ZNP
 * This API is the base of any applicative message
 * \param dst_addr The address of node on network to which we want to send a
 * message. If set to 0xFFFF, message will be broadcasted to all nodes on
 * network
 * \param dst_pan Destination PAN ID of message. If set to 0xFFFD, message will
 * be broadcast on all PAN availables on current channel
 * \param src_endpoint The id of local endpoint sending the message
 * \param dst_endpoint The destination endpoint
 * \param cluster The remote destination cluster
 * \param len The data length to send
 * \param data The data effectiveley sent to targeted system defined by address
 * - pan - endpoint - cluster
 * \param cb The callback to trigger when ZNP ahs received and processed the
 * data sending request
 */
void mt_af_send_data_request_ext(   uint64_t dst_addr,
                                    uint16_t dst_pan,
                                    uint8_t src_endpoint,
                                    uint8_t dst_endpoint,
                                    uint16_t cluster,
                                    uint16_t len,
                                    void *data,
                                    SyncActionCb cb);

#endif

