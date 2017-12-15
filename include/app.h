#ifndef APP_H
#define APP_H

#include "uv.h"

typedef enum{
    APP_STATE_INIT,
    APP_STATE_NV_CLEAR_FLAG_WRITTEN,
    APP_STATE_NV_COORD_FLAG_WRITTEN,
    APP_STATE_DONGLE_UP,
    APP_STATE_DONGLE_PRESENT,
    APP_STATE_ZLL_REGISTERED,
    APP_STATE_ZDO_STARTED,
    APP_STATE_ZLL_SCAN_REQUEST_SENT,
    APP_STATE_ZDO_DISCOVERY_SENT,
    APP_STATE_END
} AppState;

extern uv_async_t state_flag;

void reset_dongle (void);
void app_register_callbacks();
void state_machine_cb(uv_async_t *state_data);
void run_state_machine();

#endif

