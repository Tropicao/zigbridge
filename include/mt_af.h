
#ifndef MT_AF_H
#define MT_AF_H

#include <stdint.h>

void mt_af_register_callbacks(void);
void mt_af_register_zll_endpoint();
void mt_af_send_zll_scan_request();

#endif

