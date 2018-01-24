
#include <znp.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include "ipc.h"
#include "zha.h"
#include "zll.h"
#include "core.h"

/********************************
 *          Constants           *
 *******************************/

#define IPC_PIPENAME            "/tmp/zg_sock"
#define IPC_DEFAULT_VERSION     "{\"version\":\"0.3.0\"}"
#define IPC_STANDARD_ERROR      "{\"status\":\"Unknown command\"}"
/********************************
 *      Local variables         *
 *******************************/

static uv_pipe_t _server;
static uv_pipe_t _client;


/********************************
 *            Internal          *
 *******************************/

static void _send_version()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base =IPC_DEFAULT_VERSION, .len=19}
    };
    uv_write(&req, (uv_stream_t *)&_client, buffer, 1, NULL);
}

static void _send_error()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base = IPC_STANDARD_ERROR, .len=28}
    };
    uv_write(&req,(uv_stream_t *) &_client, buffer, 1, NULL);
}

static void _process_ipc_data(char *data, int len)
{
   json_t *root; 
   json_t *command;
   json_error_t error;

   if(!data || len <= 0)
   {
      LOG_ERR("Cannot process IPC command : message is corrupted"); 
      _send_error();
      return;
   }

   root = json_loads(data, JSON_DECODE_ANY, &error);
   if(!root)
   {
       LOG_ERR("Cannot decode IPC command : %s", error.text);
      _send_error();
       return;
   }

   command = json_object_get(root, "command");
   if(command && json_is_string(command))
   {
        if(strcmp(json_string_value(command), "version") == 0)
        {
            _send_version();
        }
        else
        {
            _send_error();
        }
   }
}

/********************************
 *    IPC messages callbacks    *
 *******************************/

static void _new_data_cb(uv_stream_t *s __attribute__((unused)), ssize_t n, const uv_buf_t *buf)
{
    if (n < 0)
    {
        LOG_ERR("New data size error");
        return;
    }
    
    _process_ipc_data(buf->base, n);
}

static void alloc_cb(uv_handle_t *handle __attribute__((unused)), size_t size, uv_buf_t *buf)
{
  buf->base = malloc(size);
  buf->len = size;
}

static void _new_connection_cb(uv_stream_t *s, int status)
{
    if(status)
    {
        LOG_WARN("New connection failure");
    }
    else
    {
        LOG_INF("New connection on IPC server");
        uv_pipe_init(uv_default_loop(), &_client, 0);
        if(uv_accept(s, (uv_stream_t *)&_client) == 0)
        {
            uv_read_start((uv_stream_t *)&_client, alloc_cb, _new_data_cb);
        }
        else
        {
            LOG_ERR("Error accetping new client connection");
            uv_close((uv_handle_t*) &_client, NULL);
        }
    }
}

/********************************
 *             API              *
 *******************************/

void zg_ipc_init()
{
    uv_pipe_init(uv_default_loop(), &_server, 0);
    if(uv_pipe_bind(&_server, IPC_PIPENAME))
    {
        LOG_ERR("Error binding IPC server");
        return;
    }
    if(uv_listen((uv_stream_t *)&_server, 1024, _new_connection_cb))
    {
        LOG_ERR("Error listening on IPC socket");
        return;
    }
}

void zg_ipc_shutdown()
{
    unlink(IPC_PIPENAME);
}
