
#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>
#include "device.h"
#include "interfaces.h"

typedef enum
{
    ZG_IPC_COMMAND_NONE,
    ZG_IPC_COMMAND_GET_DEVICE_LIST,
    ZG_IPC_COMMAND_TOUCHLINK,
} IpcCommand;


typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);

ZgInterfacesInterface *zg_ipc_init(void);
void zg_ipc_shutdown();

#endif

