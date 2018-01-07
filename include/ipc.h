#ifndef ZG_IPC_H
#define ZG_IPC_H

#include <uv.h>

typedef void (*ipc_fd_cb)(uv_poll_t *handler, int status, int events);

ipc_fd_cb zg_ipc_get_ipc_main_callback();

#endif

