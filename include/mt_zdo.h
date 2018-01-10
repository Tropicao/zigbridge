#ifndef ZG_MT_ZDO_H
#define ZG_MT_ZDO_H

#include <stdint.h>
#include "types.h"

void mt_zdo_register_callbacks(void);
void mt_zdo_nwk_discovery_req(SyncActionCb cb);
void mt_zdo_startup_from_app(SyncActionCb);
void mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr));

#endif

