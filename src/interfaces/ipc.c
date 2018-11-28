#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include "ipc.h"
#include "zha.h"
#include "zll.h"
#include "core.h"
#include "logs.h"
#include "utils.h"
#include "mt_zdo.h"

/********************************
 *          Constants           *
 *******************************/

#define IPC_PIPENAME            "/tmp/zg_sock"
#define IPC_DEFAULT_VERSION     "{\"version\":\"0.4.0\"}"
#define IPC_STANDARD_ERROR      "{\"status\":\"Unknown command\"}"
#define IPC_OPEN_NETWORK_OK     "{\"open_network\":\"ok\"}"
#define IPC_TOUCHLINK_OK        "{\"touchlink\":\"ok\"}"
#define IPC_TOUCHLINK_KO        "{\"touchlink\":\"error\"}"
/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static ipc_command_cb _command_cb = NULL;
static uv_pipe_t _server;
static uv_pipe_t *_client = NULL;
static char *event_strings [ZG_IPC_EVENT_GUARD] = {
    "new_device",
    "button_state",
    "event_temperature",
    "event_pressure",
    "event_humidity",
    "event_touchlink"
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

    INF("Sending version to remote client");
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

    INF("Sending error to remote client");
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

    INF("Sending device list to remote client");
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

    INF("Sending Open Network OK to IPC client");
    uv_write(req, (uv_stream_t *)_client, buffer, 1, NULL);
    zg_mt_zdo_permit_join(NULL);
}

static void _start_touchlink(void)
{
    uint8_t res;
    uv_write_t *req;
    uv_buf_t *buffer = NULL;
    char *msg = NULL;

    if(!_client)
        return;

    res = zg_zll_start_touchlink();
    req = calloc(1, sizeof(uv_write_t));
    buffer = calloc(1, sizeof(uv_buf_t));
    msg = (res == 0 ? IPC_TOUCHLINK_OK:IPC_TOUCHLINK_KO);
    buffer->base = calloc(strlen(msg)+1, sizeof(char));
    buffer->len = strlen(msg) + 1 ;
    strcpy(buffer->base, msg);
    req->data = buffer;

    INF("Sending touchlink status to IPC client");
    if(uv_write(req, (uv_stream_t *)_client, buffer, 1, _allocated_req_sent) <= 0)
        ERR("Cannot send touchlink status to IPC client");
}


static void _process_ipc_data(char *data, int len)
{
   json_t *root;
   json_t *command;
   json_error_t error;

   if(!data || len <= 0)
   {
      ERR("Cannot process IPC command : message is corrupted"); 
      _send_error();
      return;
   }

   root = json_loads(data, JSON_DECODE_ANY, &error);
   if(!root)
   {
       ERR("Cannot decode IPC command : %s", error.text);
      _send_error();
       return;
   }

   command = json_object_get(root, "command");
   if(command && json_is_string(command))
   {
        INF("IPC server received command [%s]", json_string_value(command));
        if(strcmp(json_string_value(command), "version") == 0)
            _send_version();
        else if(strcmp(json_string_value(command), "get_device_list") == 0)
            _send_device_list();
        else if(strcmp(json_string_value(command), "open_network") == 0)
            _open_network();
        else if(strcmp(json_string_value(command), "touchlink") == 0)
            _start_touchlink();
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
        ERR("User socket error");
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
        WRN("New connection failure");
    }
    else
    {
        if(_client)
        {
            WRN("Cannot accept new connection : gateway supports only one connection at once");
            return;
        }
        INF("New connection on IPC server");
        _client = (uv_pipe_t *)calloc(1, sizeof(uv_pipe_t));
        uv_pipe_init(uv_default_loop(), _client, 0);
        if(uv_accept(s, (uv_stream_t *)_client) == 0)
        {
            uv_read_start((uv_stream_t *)_client, alloc_cb, _new_data_cb);
        }
        else
        {
            ERR("Error accepting new client connection");
            uv_close((uv_handle_t*) _client, NULL);
            ZG_VAR_FREE(_client);
        }
    }
}

/********************************
 *             API              *
 *******************************/

int zg_ipc_init()
{
    _log_domain = zg_logs_domain_register("zg_ipc", ZG_COLOR_GREEN);
    uv_pipe_init(uv_default_loop(), &_server, 0);
    if(uv_pipe_bind(&_server, IPC_PIPENAME))
    {
        ERR("Error binding IPC server");
        return 1;
    }
    if(uv_listen((uv_stream_t *)&_server, 1024, _new_connection_cb))
    {
        ERR("Error listening on IPC socket");
        return 0;
    }
    INF("IPC started on socket %s", IPC_PIPENAME);
    return 0;
}

void zg_ipc_shutdown()
{
    unlink(IPC_PIPENAME);
    INF("IPC module shut down");
}

void zg_ipc_send_event(ZgIpcEvent event, json_t *data)
{
    uv_write_t *req = NULL;
    json_t *root = NULL;
    uv_buf_t *buf = NULL;
    size_t size;

    if(!data)
    {
        ERR("Cannot send event : data is empty");
        return;
    }

    if(!_client)
    {
        WRN("No active client connected, event not sent");
        return;
    }

    if(event >= ZG_IPC_EVENT_GUARD)
    {
        ERR("Invalid event");
        return;
    }

    INF("Sending event %s to remote client", event_strings[event]);
    root = json_object();
    if(!root)
    {
        ERR("Cannot create root json to send event");
        return;
    }

    if(json_object_set_new(root, "event", json_string(event_strings[event]))||
            json_object_set_new(root, "data", data))
    {
        ERR("Error encoding value into event");
        json_decref(root);
        return;
    }

    buf = calloc(1, sizeof(uv_buf_t));
    size = json_dumpb(root, NULL, 0, JSON_DECODE_ANY);
    if(size <= 0)
    {
        ERR("Cannot get size of encoded JSON");
        json_decref(root);
        return;
    }

    buf->base = calloc(size, sizeof(char));
    size = json_dumpb(root, buf->base, size, JSON_DECODE_ANY);
    json_decref(root);
    if(size <= 0)
    {
        ERR("Error printing encoded json event");
        free(buf->base);
        free(buf);
        return;
    }
    buf->len = size;
    req = calloc(1, sizeof(uv_write_t));
    req->data = buf;

    uv_write(req,(uv_stream_t *) _client, buf, 1, _allocated_req_sent);
}

void zg_ipc_register_command_cb(ipc_command_cb cb)
{
    _command_cb = cb;
}
