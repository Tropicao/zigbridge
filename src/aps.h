#ifndef ZG_APS_H
#define ZG_APS_H

#include <stdint.h>
#include "types.h"

/* APS callbacks, registered by proper applications */
typedef void (*ApsMsgCb)(uint16_t addr, uint16_t cluster, void *data, int len);

void zg_aps_init();
void zg_aps_shutdown();

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

void zg_aps_send_data(  uint16_t dst_addr,
                        uint16_t dst_pan,
                        uint8_t src_endpoint,
                        uint8_t dst_endpoint,
                        uint16_t cluster,
                        uint8_t command,
                        void *data,
                        int len,
                        SyncActionCb cb);

#endif

