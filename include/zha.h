
#ifndef ZG_ZHA_H
#define ZG_ZHA_H

#include <stdint.h>
#include "types.h"

void zg_zha_init(InitCompleteCb cb);
void zg_zha_shutdown(void);
void zg_zha_switch_bulb_state(void);
void zg_zha_set_bulb_state(uint16_t addr, uint8_t state);

#endif

