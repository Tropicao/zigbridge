#ifndef APP_H
#define APP_H

#include "uv.h"

extern uv_async_t state_flag;

void reset_dongle (void);
void app_register_callbacks();
void state_machine_cb(uv_async_t *state_data);

#endif

