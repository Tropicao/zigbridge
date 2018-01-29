
#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>
#include "device.h"

typedef enum
{
    ZG_IPC_COMMAND_NONE,
    ZG_IPC_COMMAND_GET_DEVICE_LIST,
    ZG_IPC_COMMAND_TOUCHLINK,
} IpcCommand;

typedef enum
{
    ZG_IPC_EVENT_NEW_DEVICE = 0,
    ZG_IPC_EVENT_BUTTON_STATE,
    ZG_IPC_EVENT_TEMPERATURE,
    ZG_IPC_EVENT_TOUCHLINK_FINISHED,
    ZG_IPC_EVENT_GUARD, /* Keep this value last */
} ZgIpcEvent;


typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*ipc_command_cb)(IpcCommand command);

int zg_ipc_init();
void zg_ipc_shutdown();
ipc_fd_cb zg_ipc_get_ipc_main_callback();
void zg_ipc_register_command_cb(ipc_command_cb cb);
void zg_ipc_send_event(ZgIpcEvent event, json_t *data);


#endif

