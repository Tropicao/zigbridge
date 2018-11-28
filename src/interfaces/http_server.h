
#ifndef ZG_HTTP_SERVER_H
#define ZG_HTTP_SERVER_H

#include <uv.h>
#include "device.h"
#include "interfaces.h"

ZgInterfacesInterface *zg_http_server_init(void);
void zg_http_server_shutdown(void);

#endif

