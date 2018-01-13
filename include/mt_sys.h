#ifndef ZG_MT_SYS_H
#define ZG_MT_SYS_H

#include <stdint.h>
#include "types.h"

void mt_sys_register_callbacks(void);
void mt_sys_reset_dongle(SyncActionCb cb);
void mt_sys_ping_dongle(SyncActionCb cb);
void mt_sys_nv_write_clear_state_and_config(SyncActionCb cb);
void mt_sys_nv_write_clear_config(SyncActionCb cb);
void mt_sys_nv_write_coord_flag(SyncActionCb cb);
void mt_sys_nv_write_disable_security(SyncActionCb cb);
void mt_sys_nv_write_enable_security(SyncActionCb cb);
void mt_sys_nv_set_pan_id(SyncActionCb cb);
void mt_sys_nv_write_nwk_key(SyncActionCb cb);
void mt_sys_check_ext_addr(SyncActionCb cb);
uint64_t mt_sys_get_ext_addr(void);
void mt_sys_nv_write_channel(uint8_t channel, SyncActionCb cb);

#endif

