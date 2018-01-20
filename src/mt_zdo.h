#ifndef ZG_MT_ZDO_H
#define ZG_MT_ZDO_H

#include <stdint.h>
#include "types.h"

void mt_zdo_register_callbacks(void);
void mt_zdo_nwk_discovery_req(SyncActionCb cb);
void mt_zdo_startup_from_app(SyncActionCb);
void mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr));
void mt_zdo_device_annce(uint16_t addr, uint64_t uid, SyncActionCb cb);
void mt_zdo_ext_route_disc_request(uint16_t addr, SyncActionCb cb);
void mt_zdo_query_active_endpoints(uint16_t short_addr, SyncActionCb cb);
void mt_zdo_register_active_ep_rsp_callback(void (*cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list));
void mt_zdo_register_simple_desc_rsp_cb(void (*cb)(uint8_t endpoint, uint16_t profile));
void mt_zdo_permit_join(SyncActionCb cb);

#endif

