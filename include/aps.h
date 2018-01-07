#ifndef ZG_APS_H
#define ZG_APS_H

#include <stdint.h>
#include "types.h"

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

