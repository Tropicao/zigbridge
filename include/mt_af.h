#ifndef ZG_MT_AF_H
#define ZG_MT_AF_H

#include <stdint.h>

typedef void (*zll_cb)(void *data, int len);

void mt_af_register_callbacks(void);
void mt_af_register_zll_endpoint();
void mt_af_send_zll_scan_request();
void mt_af_set_inter_pan_endpoint(void);
void mt_af_send_zll_identify_request(void);
void mt_af_send_zll_factory_reset_request(void);
void mt_af_register_zll_callback(zll_cb cb);

#endif

