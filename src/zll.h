#ifndef ZG_ZLL_H
#define ZG_ZLL_H

#include "types.h"

void zg_zll_init(InitCompleteCb cb);
void zg_zll_shutdown(void);
void zg_zll_send_scan_request(SyncActionCb cb);
void zg_zll_send_identify_request(SyncActionCb cb);
void zg_zll_send_factory_reset_request(SyncActionCb cb);
void zg_zll_start_touchlink(void);
void zg_zll_switch_bulb_state(void);

#endif

