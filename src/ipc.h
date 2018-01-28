#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>

typedef enum
{
    ZG_IPC_COMMAND_TOUCHLINK,
    ZG_IPC_COMMAND_SWITCH_DEMO_LIGHT,
    ZG_IPC_COMMAND_MOVE_TO_BLUE,
    ZG_IPC_COMMAND_MOVE_TO_RED,
    ZG_IPC_COMMAND_DISCOVERY,
    ZG_IPC_COMMAND_OPEN_NETWORK,
    ZG_IPC_COMMAND_NONE,
} IpcCommand;

typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*ipc_command_cb)(IpcCommand command);

int zg_ipc_init();
void zg_ipc_shutdown();
ipc_fd_cb zg_ipc_get_ipc_main_callback();
void zg_ipc_register_command_cb(ipc_command_cb cb);


#endif

