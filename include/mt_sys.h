#ifndef MT_SYS_H
#define MT_SYS_H

#include <stdint.h>

void mt_sys_register_callbacks(void);
void mt_sys_reset_dongle(void);
void mt_sys_ping_dongle(void);
void mt_sys_nv_write_clear_flag();
void mt_sys_nv_write_coord_flag();
void mt_sys_nv_write_disable_security(void);
void mt_sys_nv_set_pan_id(void);

#endif

