#ifndef ZG_MT_AF_H
#define ZG_MT_AF_H

#include <stdint.h>
#include "types.h"

typedef void (*ZgZllCb)(void *data, int len);

void mt_af_register_callbacks(void);
void mt_af_register_zll_endpoint(SyncActionCb cb);
void mt_af_register_zha_endpoint(SyncActionCb cb);
void mt_af_send_zll_scan_request(SyncActionCb cb);
void mt_af_set_inter_pan_endpoint(SyncActionCb cb);
void mt_af_set_inter_pan_channel(SyncActionCb cb);
void mt_af_send_zll_identify_request(SyncActionCb cb);
void mt_af_send_zll_factory_reset_request(SyncActionCb cb);
void mt_af_register_zll_callback(ZgZllCb cb);
void mt_af_switch_bulb_state(uint16_t addr, uint8_t state);

void mt_af_send_data_request_ext(   uint16_t addr,
                                    uint8_t dst_endpoint,
                                    uint16_t dst_pan,
                                    uint8_t src_endpoint,
                                    uint16_t cluster,
                                    uint16_t len,
                                    void *data,
                                    SyncActionCb cb);

#endif

