#ifndef ZG_MT_ZDO_H
#define ZG_MT_ZDO_H

#include "types.h"

void mt_zdo_register_callbacks(void);
void mt_zdo_nwk_discovery_req(SyncActionCb cb);
void mt_zdo_startup_from_app(SyncActionCb);

#endif

