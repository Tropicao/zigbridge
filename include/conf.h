#ifndef ZG_CONF_H
#define ZG_CONF_H


int zg_conf_load(char *conf_path);
void zg_conf_free();
const char *zg_conf_get_network_key_path();


#endif

