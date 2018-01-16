#ifndef ZG_DEVICES_H
#define ZG_DEVICES_H

#define ZG_DEVICE_ID_MAX    255
typedef uint8_t (DeviceId);


void zg_device_init(uint8_t reset_network);
void zg_device_shutdown();
int zg_add_device(uint16_t short_addr, uint64_t ext_addr);
uint16_t zg_device_get_short_addr(DeviceId id);
uint8_t zg_device_is_device_known(uint64_t ext_addr);

#endif

