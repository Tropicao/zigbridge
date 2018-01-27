#ifndef ZG_ZDP_H
#define ZG_ZDP_H

#include "types.h"

typedef void (*ActiveEpRspCb)(uint16_t short_addr, uint8_t nb_p, uint8_t *ep_list);
typedef void (*SimpleDescRspCb)(uint8_t endpoint, uint16_t profile, uint16_t device_id);

void zg_zdp_init(SyncActionCb cb);
void zg_zdp_shutdown(void);
void zg_zdp_query_active_endpoints(uint16_t short_addr, SyncActionCb cb);
void zg_zdp_query_simple_descriptor(uint16_t short_addr, uint8_t endpoint, SyncActionCb cb);
void zg_zdp_register_active_endpoints_rsp(ActiveEpRspCb cb);
void zg_zdp_register_simple_desc_rsp(SimpleDescRspCb cb);

#endif
