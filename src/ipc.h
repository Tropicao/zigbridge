
#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>
#include "device.h"

typedef enum
{
    ZG_IPC_COMMAND_NONE,
    ZG_IPC_COMMAND_GET_DEVICE_LIST,
} IpcCommand;

typedef enum
{
    ZG_IPC_EVENT_NEW_DEVICE,
    ZG_IPC_EVENT_BUTTON_STATE,
    ZG_IPC_EVENT_GUARD, /* Keep this value last */
} ZgIpcEvent;


typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*ipc_command_cb)(IpcCommand command);

void zg_ipc_init();
void zg_ipc_shutdown();
void zg_ipc_send_event(ZgIpcEvent event, json_t *data);


#endif

