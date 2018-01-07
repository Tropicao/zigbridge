#ifndef ZG_MT_AF_H
#define ZG_MT_AF_H

#include <stdint.h>
#include "types.h"

typedef void (*AfIncomingMessageCb)(uint8_t endpoint, void *data, int len);

void mt_af_register_callbacks(void);
void mt_af_register_endpoint(   uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                SyncActionCb cb);
void mt_af_register_zha_endpoint(SyncActionCb cb);
void mt_af_set_inter_pan_endpoint(SyncActionCb cb);
void mt_af_set_inter_pan_channel(SyncActionCb cb);
void mt_af_register_incoming_message_callback(AfIncomingMessageCb cb);
void mt_af_switch_bulb_state(uint16_t addr, uint8_t state);

void mt_af_send_data_request_ext(   uint16_t dst_addr,
                                    uint16_t dst_pan,
                                    uint8_t src_endpoint,
                                    uint8_t dst_endpoint,
                                    uint16_t cluster,
                                    uint16_t len,
                                    void *data,
                                    SyncActionCb cb);

#endif

