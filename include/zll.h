#ifndef ZG_ZLL_H
#define ZG_ZLL_H

#include "types.h"


typedef void (*InitCompleteCb)(void);

void zg_zll_init(InitCompleteCb cb);
void zg_zll_send_scan_request(SyncActionCb cb);
void zg_zll_register_scan_response_callback(void);
void zg_zll_send_identify_request(SyncActionCb cb);
void zg_zll_send_factory_reset_request(SyncActionCb cb);
void zg_zll_send_join_router_request(void);
void zg_zll_start_touchlink(void);
void zg_zll_switch_bulb_state(void);

#endif

