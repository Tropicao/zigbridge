#ifndef MT_SYS_H
#define MT_SYS_H

#include <stdint.h>

void mt_sys_register_callbacks(void);
void mt_sys_reset_dongle(void);
void mt_sys_ping_dongle(void);
void mt_sys_osal_nv_write(uint16_t id, uint8_t offset, uint8_t length, uint8_t *data);

#endif

