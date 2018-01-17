#ifndef ZG_ZDP_H
#define ZG_ZDP_H

#include "types.h"

void zg_zdp_init(SyncActionCb cb);
void zg_zdp_shutdown(void);
void zg_zdp_query_active_endpoints(uint16_t short_addr, SyncActionCb cb);

#endif
