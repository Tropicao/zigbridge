
#include <znp.h>
#include <unistd.h>
#include <string.h>
#include "ipc.h"
#include "zha.h"
#include "zll.h"
#include "core.h"

/********************************
 *          Constants           *
 *******************************/

#define IPC_PIPENAME        "zg_sock"

/********************************
 *      Local variables         *
 *******************************/

static uv_pipe_t server;


/********************************
 *            Internal          *
 *******************************/

/********************************
 *    IPC messages callbacks    *
 *******************************/

static void _new_connection_cb(uv_stream_t *s __attribute__((unused)), int status)
{
    if(status)
        LOG_WARN("New connection failure");
    else
        LOG_INF("New connection on IPC server");
}

/********************************
 *             API              *
 *******************************/

void zg_ipc_init()
{
    uv_pipe_init(uv_default_loop(), &server, 0);
    if(uv_pipe_bind(&server, IPC_PIPENAME))
    {
        LOG_ERR("Error binding IPC server");
        return;
    }
    if(uv_listen((uv_stream_t *)&server, 1024, _new_connection_cb))
    {
        LOG_ERR("Error listening on IPC socket");
        return;
    }
}

void zg_ipc_shutdown()
{

}
