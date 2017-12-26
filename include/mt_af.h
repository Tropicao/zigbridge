#ifndef ZG_MT_AF_H
#define ZG_MT_AF_H

#include <stdint.h>
#include "types.h"

typedef void (*ZgZllCb)(void *data, int len);

void mt_af_register_callbacks(void);
void mt_af_register_zll_endpoint(SyncActionCb cb);
void mt_af_send_zll_scan_request(SyncActionCb cb);
void mt_af_set_inter_pan_endpoint(SyncActionCb cb);
void mt_af_send_zll_identify_request(SyncActionCb cb);
void mt_af_send_zll_factory_reset_request(SyncActionCb cb);
void mt_af_register_zll_callback(ZgZllCb cb);

#endif

