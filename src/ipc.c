#include <unistd.h>
#include <string.h>
#include "ipc.h"
#include "zha.h"
#include "zll.h"
#include "core.h"
#include "logs.h"

/********************************
 *          Constants           *
 *******************************/


/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static ipc_command_cb _command_cb = NULL;


/********************************
 *            Internal          *
 *******************************/

/********************************
 *    IPC messages callbacks    *
 *******************************/

static void _process_ipc_message_cb(uv_poll_t *handler __attribute__((unused)), int status, int events)
{
    uint8_t buffer[256] = {0};
    int size = 0;
    int index = 0;
    IpcCommand cmd = ZG_IPC_COMMAND_NONE;

    if(status < 0)
    {
        ERR("User socket error %s (%s)", uv_err_name(status), uv_strerror(status));
        return;
    }

    if(events & UV_READABLE)
    {
        do
        {
            size = read(0, buffer + index, 256);
            if(size > 0 && size + index < 255)
                index += size;
        } while (size > 0);

        if(buffer[0] == '\n')
        {
            cmd = ZG_IPC_COMMAND_SWITCH_DEMO_LIGHT;
        }
        else
        {
            buffer[size > 0 ? index+size-1:index-1] = 0;
            if(strcmp((char *)buffer, "touchlink") == 0)
                cmd = ZG_IPC_COMMAND_TOUCHLINK;
            else if(strcmp((char *)buffer, "d") == 0)
                cmd = ZG_IPC_COMMAND_DISCOVERY;
            else if(strcmp((char*)buffer, "o") == 0)
                cmd = ZG_IPC_COMMAND_OPEN_NETWORK;
            else if(strcmp((char*)buffer, "b") == 0)
                cmd = ZG_IPC_COMMAND_MOVE_TO_BLUE;
            else if(strcmp((char*)buffer, "r") == 0)
                cmd = ZG_IPC_COMMAND_MOVE_TO_RED;
            else
            {
                WRN("Unknown command");
            }
        }
        if(cmd != ZG_IPC_COMMAND_NONE && _command_cb)
            _command_cb(cmd);
    }
}

/********************************
 *             API              *
 *******************************/

int zg_ipc_init()
{
    _log_domain = zg_logs_domain_register("zg_ipc", ZG_COLOR_BLACK);
    INF("IPC module initialized");
    return 0;
}

void zg_ipc_shutdown()
{
    INF("IPC module shut down");
}

ipc_fd_cb zg_ipc_get_ipc_main_callback()
{
    return _process_ipc_message_cb;
}

void zg_ipc_register_command_cb(ipc_command_cb cb)
{
    _command_cb = cb;
}
