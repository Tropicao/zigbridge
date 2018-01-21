
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
void zg_zha_register_button_state_cb(void (*cb)(void));
void zg_zha_register_temperature_cb(void (*cb)(uint16_t temp));
void zg_zha_move_to_color(uint16_t short_addr, uint16_t x, uint16_t y, uint8_t duration_s);

#endif

