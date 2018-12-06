#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include "interfaces.h"
#include "logs.h"
#include "conf.h"
#include "tcp.h"
#include "zha.h"
#include "zll.h"
#include "core.h"
#include "logs.h"
#include "utils.h"
#include "mt_zdo.h"

/********************************
 *          Macros              *
 *******************************/

#define CLOSE_CLIENT(x)                             do {\
                                                        if(x){\
                                                            uv_read_stop((uv_stream_t *)x);\
                                                            uv_close((uv_handle_t *)x, NULL);\
                                                            ZG_VAR_FREE(x);\
                                                        }} while(0);
/********************************
 *          Constants           *
 *******************************/

#define MAX_PENDING_CONNECTTION         2

#define TCP_DEFAULT_VERSION     "{\"version\":\"0.4.0\"}"
#define TCP_STANDARD_ERROR      "{\"status\":\"Unknown command\"}"
#define TCP_OPEN_NETWORK_OK     "{\"open_network\":\"ok\"}"
#define TCP_TOUCHLINK_OK        "{\"touchlink\":\"ok\"}"
#define TCP_TOUCHLINK_KO        "{\"touchlink\":\"error\"}"
/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static uv_tcp_t _server_handle;
static uv_tcp_t *_client_handle = NULL;
static ZgInterfacesInterface *_interface = NULL;

/********************************
 *            Internal          *
 *******************************/

static void _allocated_req_sent(uv_write_t *req, int status __attribute__((unused)))
{
    uv_buf_t *buf = (uv_buf_t *)req->data;

    free(buf->base);
    free(buf);
}

/********************************
 *    TCP messages callbacks    *
 *******************************/

static void _send_error()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base = TCP_STANDARD_ERROR, .len=28}
    };

    if(!_client_handle)
        return;

    INF("Sending error to remote client");
    uv_write(&req,(uv_stream_t *) _client_handle, buffer, 1, NULL);
}

/*static void _send_version()
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base = TCP_DEFAULT_VERSION, .len=19}
    };

    if(!_client_handle)
        return;

    INF("Sending version to remote client");
    uv_write(&req, (uv_stream_t *)_client_handle, buffer, 1, NULL);
}

static void _send_device_list()
{
    uv_write_t *req = calloc(1, sizeof(uv_write_t));
    size_t size;
    json_t *devices = zg_device_get_device_list_json();

    if(!_client_handle)
        return;

    INF("Sending device list to remote client");
    uv_buf_t *buf = calloc(1, sizeof(uv_buf_t));
    size = json_dumpb(devices, NULL, 0, JSON_DECODE_ANY);
    buf->base = calloc(size, sizeof(char));
    size = json_dumpb(devices, buf->base, size, JSON_DECODE_ANY);
    json_decref(devices);
    buf->len = size;
    req->data = buf;

    uv_write(req,(uv_stream_t *) _client_handle, buf, 1, _allocated_req_sent);
}

static void _open_network(void)
{
    uv_write_t req;
    uv_buf_t buffer[] = {
        {.base =TCP_OPEN_NETWORK_OK, .len=21}
    };

    if(!_client_handle)
        return;

    INF("Sending Open Network OK to IPC client");
    uv_write(&req, (uv_stream_t *)_client_handle, buffer, 1, NULL);
    zg_mt_zdo_permit_join(NULL);
}

static void _start_touchlink(void)
{
    uint8_t res;
    uv_write_t *req = calloc(1, sizeof(uv_write_t));
    uv_buf_t *buffer = NULL;
    char *msg = NULL;

    if(!_client_handle)
        return;

    res = zg_zll_start_touchlink();
    buffer = calloc(1, sizeof(uv_buf_t));
    msg = (res == 0 ? TCP_TOUCHLINK_OK:TCP_TOUCHLINK_KO);
    buffer->base = calloc(strlen(msg)+1, sizeof(char));
    buffer->len = strlen(msg) + 1 ;
    strcpy(buffer->base, msg);
    req->data = buffer;

    INF("Sending touchlink status to IPC client");
    if(uv_write(req, (uv_stream_t *)_client_handle, buffer, 1, _allocated_req_sent) <= 0)
        ERR("Cannot send touchlink status to IPC client");
}
*/

static void _dispatch_answer(ZgInterfacesAnswerObject *obj)
{
    uv_write_t *req = NULL;
    json_t *devices = zg_device_get_device_list_json();

    if(!_client_handle || !obj || !(obj->data))
    {
        return;
    }

    INF("Sending command answer");
    req = calloc(1, sizeof(uv_write_t));
    uv_buf_t *buf = calloc(1, sizeof(uv_buf_t));
    buf->base = calloc(obj->len, sizeof(char));
    strncpy(buf->base, obj->data, obj->len);
    json_decref(devices);
    buf->len = obj->len;
    DBG("Data : [%s] (%zd)", buf->base, buf->len);
    req->data = buf;

    uv_write(req,(uv_stream_t *) _client_handle, buf, 1, _allocated_req_sent);
}
static void _process_tcp_data(char *data, int len)
{
   json_t *root;
   json_t *command;
   json_error_t error;
   ZgInterfacesCommandObject *command_obj = NULL;
   ZgInterfacesAnswerObject *answer_obj = NULL;

   if(!data || len <= 0)
   {
      ERR("Cannot process TCP command : message is corrupted");
      _send_error();
      return;
   }

   root = json_loads(data, JSON_DECODE_ANY, &error);
   if(!root)
   {
       ERR("Cannot decode TCP command : %s", error.text);
      _send_error();
       return;
   }

   command = json_object_get(root, "command");
   if(command && json_is_string(command))
   {
        INF("TCP server received command [%s]", json_string_value(command));
        CALLOC_COMMAND_OBJ_RET(command_obj);
        strcpy(command_obj->command_string, json_string_value(json_object_get(root, "command")));
        command_obj->data = json_object_get(root, "data");
        command_obj->len = 0; /* Need to better define Command object */
        answer_obj = zg_interfaces_process_command(_interface, command_obj);
        _dispatch_answer(answer_obj);
        zg_interfaces_free_command_object(command_obj);
        zg_interfaces_free_answer_object(answer_obj);
   }
}

static void _new_data_cb(uv_stream_t *s __attribute__((unused)), ssize_t n, const uv_buf_t *buf)
{
    if (n < 0)
    {
        ERR("User socket error");
        CLOSE_CLIENT(_client_handle);
        return;
    }

    _process_tcp_data(buf->base, n);
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
        WRN("New client connection failure");
        return;
    }

    if(_client_handle)
    {
        WRN("Cannot accept new connection : gateway supports only one TCP connection at once for now");
        return;
    }

    INF("New connection on tcp server");
    _client_handle = (uv_tcp_t *)calloc(1, sizeof(uv_tcp_t));

    if(!_client_handle || uv_tcp_init(uv_default_loop(), _client_handle) !=0)
    {
        ERR("Cannot initiate new client structure");
        ZG_VAR_FREE(_client_handle);
    }

    if(uv_accept(s, (uv_stream_t *)_client_handle) != 0 ||
            uv_read_start((uv_stream_t *)_client_handle, alloc_cb, _new_data_cb) != 0)
    {
        ERR("Error on accepting new client");
        CLOSE_CLIENT(_client_handle);
    }
}

static void _send_event(uv_buf_t *buf)
{
    uv_write_t *req = NULL;
    uv_buf_t *local_buf = NULL;

    if(!buf || !(buf->base) || !(buf->len))
    {
        ERR("Event to dispatch is empty");
        return;
    }
    local_buf = calloc(1, sizeof(uv_buf_t));

    local_buf->base = calloc(buf->len, sizeof(char));
    memcpy(local_buf->base, buf->base, buf->len);
    local_buf->len = buf->len;
    req = calloc(1, sizeof(uv_write_t));
    req->data = local_buf;

    DBG("Dispatching new event on TCP interface");
    if(_client_handle)
        uv_write(req,(uv_stream_t *) _client_handle, local_buf, 1, _allocated_req_sent);
}

/********************************
 *             API              *
 *******************************/

ZgInterfacesInterface *zg_tcp_init()
{
    struct sockaddr_in bind_addr;

    if(_init_count != 0)
    {
        return NULL;
    }

    _log_domain = zg_logs_domain_register("zg_tcp", ZG_COLOR_GREEN);

    if(uv_tcp_init(uv_default_loop(), &_server_handle) != 0)
    {
        ERR("Cannot initialize HTTP server handle");
        return NULL;
    }
    if(uv_ip4_addr(zg_conf_get_tcp_server_address(),zg_conf_get_tcp_server_port(), & bind_addr) !=0 ||
            uv_tcp_bind(&_server_handle, (struct sockaddr *)&bind_addr, 0) != 0)
    {
        ERR("Cannot bind HTTP server socket");
        return NULL;
    }

    if(uv_listen((uv_stream_t *)&_server_handle, MAX_PENDING_CONNECTTION, _new_connection_cb) != 0)
    {
        ERR("Cannot start listening for new connection");
        return NULL;
    }
    _interface = calloc(1, sizeof(ZgInterfacesInterface));
    sprintf((char *)_interface->name, "TCP");
    _interface->event_cb = _send_event;

    INF("TCP server started on address %s - port %d", zg_conf_get_tcp_server_address(), zg_conf_get_tcp_server_port());
    _init_count = 1;

    return _interface;
}

void zg_tcp_shutdown()
{
    if(_init_count != 1)
    {
        return;
    }

    ZG_VAR_FREE(_interface);
    CLOSE_CLIENT(_client_handle);
    _init_count--;
    INF("TCP module shut down");
}

