#include <znp.h>
#include <unistd.h>
#include "ipc.h"
#include "zha.h"

/********************************
 *          Constants           *
 *******************************/


/********************************
 *      Local variables         *
 *******************************/


/********************************
 *            Internal          *
 *******************************/

/********************************
 *    IPC messages callbacks    *
 *******************************/

static void _process_ipc_message_cb(uv_poll_t *handler __attribute__((unused)), int status, int events)
{
    uint8_t buffer[16];
    if(status < 0)
    {
        LOG_ERR("User socket error %s (%s)", uv_err_name(status), uv_strerror(status));
        return;
    }

    if(events & UV_READABLE)
    {
        LOG_DBG("User has pressed enter");
        do {} while(read(0, buffer, 16) > 0);
        zg_zha_switch_bulb_state();
    }
}

/********************************
 *             API              *
 *******************************/

ipc_fd_cb zg_ipc_get_ipc_main_callback()
{
    return _process_ipc_message_cb;
}
