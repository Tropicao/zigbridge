#ifndef ZG_MT_SYS_H
#define ZG_MT_SYS_H

#include <stdint.h>
#include "types.h"

typedef enum
{
    STARTUP_NO_CLEAR =                  0x00,
    STARTUP_CLEAR_NWK_FRAME_COUNTER =   0x01,
    STARTUP_CLEAR_STATE =               0x02,
    STARTUP_CLEAR_CONFIG =              0x04,
} MtSysStartupOptions;

void mt_sys_register_callbacks(void);
void mt_sys_reset_dongle(SyncActionCb cb);
void mt_sys_ping_dongle(SyncActionCb cb);
void mt_sys_nv_write_startup_options(MtSysStartupOptions options, SyncActionCb cb);
void mt_sys_nv_write_coord_flag(SyncActionCb cb);
void mt_sys_nv_write_disable_security(SyncActionCb cb);
void mt_sys_nv_write_enable_security(SyncActionCb cb);
void mt_sys_nv_set_pan_id(SyncActionCb cb);
void mt_sys_nv_write_nwk_key(SyncActionCb cb);
void mt_sys_check_ext_addr(SyncActionCb cb);
uint64_t mt_sys_get_ext_addr(void);
void mt_sys_nv_write_channel(uint8_t channel, SyncActionCb cb);

#endif

