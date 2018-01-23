
#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>
#include "device.h"

typedef enum
{
    ZG_IPC_COMMAND_NONE,
    ZG_IPC_COMMAND_GET_DEVICE_LIST,
} IpcCommand;


typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*ipc_command_cb)(IpcCommand command);

void zg_ipc_init();
void zg_ipc_shutdown();


#endif

