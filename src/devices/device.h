#ifndef ZG_DEVICES_H
#define ZG_DEVICES_H

#include <jansson.h>

#define ZG_DEVICE_ID_MAX    255
typedef uint8_t (DeviceId);


int zg_device_init(uint8_t reset_network);
void zg_device_shutdown();
int zg_add_device(uint16_t short_addr, uint64_t ext_addr);
uint16_t zg_device_get_short_addr(DeviceId id);
DeviceId zg_device_get_id(uint16_t short_addr);
uint8_t zg_device_is_device_known(uint64_t ext_addr);
void zg_device_update_endpoints(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list);
void zg_device_update_endpoint_data(uint16_t addr, uint8_t endpoint, uint16_t profile, uint16_t device_id);
uint8_t zg_device_get_next_empty_endpoint(uint16_t addr);
json_t *zg_device_get_device_list_json(void);

#endif

