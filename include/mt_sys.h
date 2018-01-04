#ifndef ZG_MT_SYS_H
#define ZG_MT_SYS_H

#include <stdint.h>
#include "types.h"

void mt_sys_register_callbacks(void);
void mt_sys_reset_dongle(SyncActionCb cb);
void mt_sys_ping_dongle(SyncActionCb cb);
void mt_sys_nv_write_clear_flag(SyncActionCb cb);
void mt_sys_nv_write_coord_flag(SyncActionCb cb);
void mt_sys_nv_write_disable_security(SyncActionCb cb);
void mt_sys_nv_write_enable_security(SyncActionCb cb);
void mt_sys_nv_set_pan_id(SyncActionCb cb);
void mt_sys_nv_write_nwk_key(SyncActionCb cb);

#endif

