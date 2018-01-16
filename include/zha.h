
#ifndef ZG_ZHA_H
#define ZG_ZHA_H

#include <stdint.h>
#include "types.h"

typedef void (*NewDeviceJoinedCb)(uint16_t short_addr, uint64_t ext_addr);

void zg_zha_init(InitCompleteCb cb);
void zg_zha_shutdown(void);
void zg_zha_switch_bulb_state(uint16_t short_addr);
void zg_zha_set_bulb_state(uint16_t addr, uint8_t state);
void zg_zha_register_device_ind_callback(NewDeviceJoinedCb cb);
void zha_ask_node_descriptor(uint16_t short_addr);
void zg_zha_query_active_endpoints(uint16_t short_addr, SyncActionCb cb);

#endif

