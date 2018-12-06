
#ifndef ZG_ZHA_H
#define ZG_ZHA_H

#include <stdint.h>
#include "types.h"

typedef void (*NewDeviceJoinedCb)(uint16_t short_addr, uint64_t ext_addr);

uint8_t zg_zha_init(InitCompleteCb cb);
void zg_zha_shutdown(void);
void zg_zha_on_off_set(uint16_t addr, uint8_t endpoint, uint8_t state);
void zg_zha_register_device_ind_callback(NewDeviceJoinedCb cb);
void zg_zha_register_button_state_cb(void (*cb)(uint16_t short_addr, uint8_t state));
void zg_zha_register_temperature_cb(void (*cb)(uint16_t short_addr, int16_t temp));
void zg_zha_register_humidity_cb(void (*cb)(uint16_t short_addr, uint16_t humidity));
void zg_zha_register_pressure_cb(void (*cb)(uint16_t short_addr, int16_t humidity));
void zg_zha_move_to_color(uint16_t short_addr, uint16_t x, uint16_t y, uint8_t duration_s);

#endif

