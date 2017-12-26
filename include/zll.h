#ifndef ZG_ZLL_H
#define ZG_ZLL_H

void zg_zll_init(void);
void zg_zll_send_scan_request(void);
void zg_zll_register_scan_response_callback(void);
void zg_zll_send_identify_request(void);
void zg_zll_send_factory_reset_request(void);
void zg_zll_send_join_router_request(void);

#endif

