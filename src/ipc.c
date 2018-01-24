
#include <znp.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include "ipc.h"
#include "zha.h"
#include "zll.h"
#include "core.h"
#include "utils.h"
#include "mt_zdo.h"

/********************************
 *          Constants           *
 *******************************/

#define IPC_PIPENAME            "/tmp/zg_sock"
#define IPC_DEFAULT_VERSION     "{\"version\":\"0.4.0\"}"
#define IPC_STANDARD_ERROR      "{\"status\":\"Unknown command\"}"
#define IPC_OPEN_NETWORK_OK     "{\"open_network\":\"ok\"}"
/********************************
 *      Local variables         *
 *******************************/

static uv_pipe_t _server;
static uv_pipe_t *_client = NULL;
static char *event_strings [] = {
    "new_device",
    "button_state",
};


/********************************
 *            Internal          *
 *******************************/

static void _send_version()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base =IPC_DEFAULT_VERSION, .len=19}
    };

    if(!_client)
        return;

    LOG_INF("Sending version to remote client");
    uv_write(&req, (uv_stream_t *)_client, buffer, 1, NULL);
}

static void _send_error()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base = IPC_STANDARD_ERROR, .len=28}
    };

    if(!_client)
        return;

    LOG_INF("Sending error to remote client");
    uv_write(&req,(uv_stream_t *) _client, buffer, 1, NULL);
}

static void _allocated_req_sent(uv_write_t *req, int status __attribute__((unused)))
{
    uv_buf_t *buf = (uv_buf_t *)req->data;

    free(buf->base);
    free(buf);
}

static void _send_device_list()
{
    uv_write_t req;
    size_t size;
    json_t *devices = zg_device_get_device_list_json();

    if(!_client)
        return;

    LOG_INF("Sending device list to remote client");
    uv_buf_t *buf = calloc(1, sizeof(uv_buf_t));
    size = json_dumpb(devices, NULL, 0, JSON_DECODE_ANY);
    buf->base = calloc(size, sizeof(char));
    size = json_dumpb(devices, buf->base, size, JSON_DECODE_ANY);
    json_decref(devices);
    buf->len = size;
    req.data = buf;

    uv_write(&req,(uv_stream_t *) _client, buf, 1, _allocated_req_sent);
}

static void _open_network(void)
{
    uv_write_t *req;
    uv_buf_t buffer[] = {
        {.base =IPC_OPEN_NETWORK_OK, .len=21}
    };

    if(!_client)
        return;
    req = calloc(1, sizeof(uv_write_t));

    LOG_INF("Sending Open Network OK to IPC client");
    uv_write(req, (uv_stream_t *)_client, buffer, 1, NULL);
    mt_zdo_permit_join(NULL);
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
        LOG_INF("IPC server received command [%s]", json_string_value(command));
        if(strcmp(json_string_value(command), "version") == 0)
            _send_version();
        else if(strcmp(json_string_value(command), "get_device_list") == 0)
            _send_device_list();
        else if(strcmp(json_string_value(command), "open_network") == 0)
            _open_network();
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
        if(_client)
        {
            LOG_WARN("Cannot accept new connection : gateway supports only one connection at once");
            return;
        }
        LOG_INF("New connection on IPC server");
        _client = (uv_pipe_t *)calloc(1, sizeof(uv_pipe_t));
        uv_pipe_init(uv_default_loop(), _client, 0);
        if(uv_accept(s, (uv_stream_t *)_client) == 0)
        {
            uv_read_start((uv_stream_t *)_client, alloc_cb, _new_data_cb);
        }
        else
        {
            LOG_ERR("Error accepting new client connection");
            uv_close((uv_handle_t*) _client, NULL);
            ZG_VAR_FREE(_client);
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
    LOG_INF("IPC started on socket %s", IPC_PIPENAME);
}

void zg_ipc_shutdown()
{
    unlink(IPC_PIPENAME);
}

void zg_ipc_send_event(ZgIpcEvent event, json_t *data)
{
    uv_write_t *req = NULL;
    json_t *root = NULL;
    uv_buf_t *buf = NULL;
    size_t size;

    if(!data)
    {
        LOG_ERR("Cannot send event : data is empty");
        return;
    }

    if(!_client)
    {
        LOG_WARN("No active client connected, event not sent");
        return;
    }

    if(event >= ZG_IPC_EVENT_GUARD)
    {
        LOG_ERR("Invalid event");
        return;
    }

    LOG_INF("Sending event %s to remote client", event_strings[event]);
    root = json_object();
    if(!root)
    {
        LOG_ERR("Cannot create root json to send event");
        return;
    }

    if(json_object_set_new(root, "event", json_string(event_strings[event]))||
            json_object_set_new(root, "data", data))
    {
        LOG_ERR("Error encoding value into event");
        json_decref(root);
        return;
    }

    buf = calloc(1, sizeof(uv_buf_t));
    size = json_dumpb(root, NULL, 0, JSON_DECODE_ANY);
    if(size <= 0)
    {
        LOG_ERR("Cannot get size of encoded JSON");
        json_decref(root);
    }

    buf->base = calloc(size, sizeof(char));
    size = json_dumpb(root, buf->base, size, JSON_DECODE_ANY);
    json_decref(root);
    if(size <= 0)
    {
        LOG_ERR("Error printing encoded json event");
        free(buf->base);
        free(buf);
        return;
    }
    buf->len = size;
    req = calloc(1, sizeof(uv_write_t));
    req->data = buf;
    if(req->data)
        LOG_INF("Sending data : %s", buf->base);

    uv_write(req,(uv_stream_t *) _client, buf, 1, _allocated_req_sent);
}

