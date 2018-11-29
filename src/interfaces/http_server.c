#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "http_server.h"
#include "core.h"
#include "logs.h"
#include "utils.h"
#include "conf.h"
#include "http_parser.h"
#include "interfaces.h"

/********************************
 *          Constants           *
 *******************************/

#define CLOSE_CLIENT(x)                             do {\
                                                        if(x){\
                                                            uv_read_stop((uv_stream_t *)x);\
                                                            uv_close((uv_handle_t *)x, NULL);\
                                                            ZG_VAR_FREE(x);\
                                                        }} while(0);

#define DEINIT_HTTP_PARSER(x)                       do {\
                                                        ZG_VAR_FREE(x);\
                                                    } while(0);

#define FILL_HTTP_BUFFER(msg, index, data, size)     do {\
                                                        if(len <= 0){\
                                                            ERR("Invalid data length");\
                                                        } else {\
                                                            msg[index].base = calloc(size + 1, sizeof(char));\
                                                            memcpy(msg[index].base, data, size);\
                                                            msg[index].len = size;\
                                                    }} while(0);

#define HTTP_SERVER_PIPENAME            "/tmp/zg_sock"
#define MAX_PENDING_CONNECTTION         2

#define MAX_URL_SIZE                    512

/* STUB data */
#define HTTP_VERSION_STR           "HTTP/1.1"
#define HTTP_CONTENT_LEN_STR       "Content-Length"
#define HTTP_CONTENT_TYPE_STR      "Content-Type"

typedef enum
{
    HTTP_FIELD_STATUS,
    HTTP_FIELD_CONTENT_LEN,
    HTTP_FIELD_CONTENT_TYPE,
    HTTP_FIELD_SPACER,
    HTTP_FIELD_DATA,
    HTTP_FIELD_MAX
} HttpField;


/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static uv_tcp_t _server_handle;
static uv_tcp_t *_client_handle = NULL;
static http_parser_settings _hp_settings;
static http_parser *_hp_parser = NULL;
static ZgInterfacesInterface _interface;

/********************************
 *      Structures and data     *
 *******************************/


/********************************
 *            Internal          *
 *******************************/

static void _dump_http_answer(uv_buf_t *answer)
{
    int i = 0;

    if(!answer)
    {
        ERR("Cannot dump HTTP answer : object is NULL");
        return;
    }

    DBG("Dump HTTP answer : ");
    for(i = 0; i < HTTP_FIELD_MAX; i++)
    {
        DBG("==> Field %d : %s (%zd)", i, answer[i].base, answer[i].len);
    }
}

static void _free_http_answer(uv_buf_t *msg)
{
    int i = 0;
    if(msg)
    {
        for(i = 0; i < HTTP_FIELD_MAX; i++)
        {
            ZG_VAR_FREE(msg[i].base);
        }
        ZG_VAR_FREE(msg);
    }
}

static uv_buf_t *_build_http_answer(ZgInterfacesAnswerObject *obj)
{
    uv_buf_t *res = NULL;
    char buffer[512] = {0};
    int len;

    if(!obj)
        return res;

    res = calloc(HTTP_FIELD_MAX, sizeof(uv_buf_t));
    if(!res)
        return res;

    len = sprintf(buffer, "%s %d %s\r\n", HTTP_VERSION_STR, obj->status ? 400:200, obj->status ? "Not found":"OK");
    FILL_HTTP_BUFFER(res, HTTP_FIELD_STATUS, buffer, len);
    len = sprintf(buffer, "%s: %s\r\n", HTTP_CONTENT_TYPE_STR, "text/ascii");
    FILL_HTTP_BUFFER(res, HTTP_FIELD_CONTENT_TYPE, buffer, len);
    len = sprintf(buffer, "%s: %d\r\n", HTTP_CONTENT_LEN_STR, obj->len);
    FILL_HTTP_BUFFER(res, HTTP_FIELD_CONTENT_LEN, buffer, len);
    len = sprintf(buffer, "\r\n");
    FILL_HTTP_BUFFER(res, HTTP_FIELD_SPACER, buffer, len);
    FILL_HTTP_BUFFER(res, HTTP_FIELD_DATA, obj->data, obj->len);

    return res;
}

static void _client_answer_cb(uv_write_t *req __attribute__((unused)), int status)
{
    if(status)
    {
        ERR("Client answer has failed");
    }
    _free_http_answer((uv_buf_t*)req->data);
    ZG_VAR_FREE(req);
}

static void _dispatch_answer(ZgInterfacesAnswerObject *obj)
{
    uv_buf_t *msg = NULL;
    if(!obj)
        return;

    uv_write_t *req = calloc(1, sizeof(uv_write_t));
    msg = _build_http_answer(obj);
    if(!req || !obj ||!msg)
    {
        ERR("Cannot build HTTP answer");
        ZG_VAR_FREE(req);
        ZG_VAR_FREE(obj);
        _free_http_answer(msg);
        return;
    }

    _dump_http_answer(msg);
    req->data = msg;
    if(uv_write(req, (uv_stream_t *)_client_handle, msg, HTTP_FIELD_MAX, _client_answer_cb) != 0)
    {
        ERR("Error writing to client");
    }
    INF("Answer dispatched to client (status %d)", obj->status);
}

static int _url_asked_cb(http_parser *_hp __attribute__((unused)), const char *at __attribute__((unused)), size_t len __attribute__((unused)))
{

    struct http_parser_url url;
    ZgInterfacesCommandObject *command_obj = NULL;
    ZgInterfacesAnswerObject  *answer_obj = NULL;

    if(len >= MAX_URL_SIZE)
    {
        ERR("Cannot parse too big url (limitation set to %d)", MAX_URL_SIZE);
        return 1;
    }

    DBG("URL event (size %zd)", len);
    http_parser_url_init(&url);
    if(http_parser_parse_url(at, len, 0, &url) != 0)
    {
        ERR("Failed to parse calling URL");
        CLOSE_CLIENT(_client_handle);
        DEINIT_HTTP_PARSER(_hp_parser);
        return 1;
    }

    if(url.field_set & (1 << UF_PATH) && url.field_data[UF_PATH].len > 0)
    {
        DBG("Found URL path : %.*s", url.field_data[UF_PATH].len, at + url.field_data[UF_PATH].off);

        CALLOC_COMMAND_OBJ_RET(command_obj, 1);
        memcpy(command_obj->command_string, at + url.field_data[UF_PATH].off + 1, url.field_data[UF_PATH].len);
        answer_obj = zg_interfaces_process_command(&_interface, command_obj);
        _dispatch_answer(answer_obj);
        zg_interfaces_free_command_object(command_obj);
        zg_interfaces_free_answer_object(answer_obj);
    }

    return 0;
}

/****************************************
 *    HTTP_SERVER messages callbacks    *
 ****************************************/

static void _new_data_cb(uv_stream_t *s __attribute__((unused)), ssize_t n, const uv_buf_t *buf)
{
    size_t nparsed ;
    const char *data = NULL;

    if (n < 0)
    {
        if(n == UV_EOF)
            INF("Client disconnected from HTTP server");
        else
            ERR("User socket error (%zd)", n);
        goto new_data_end_error;
    }

    for(;;)
    {
        nparsed = http_parser_execute(_hp_parser, &_hp_settings, buf->base, n);
        if(_hp_parser->http_errno != HPE_OK)
        {
            WRN("Cannot parse client request : %s", http_errno_description(_hp_parser->http_errno));
            goto new_data_end_error;
        }

        if(nparsed == (size_t)n)
        {
            DBG("All HTTP data has been properly parsed");
            break;
        }

        data += nparsed;
        n -= nparsed;
    }

    free(buf->base);
    return;
new_data_end_error:
    CLOSE_CLIENT(_client_handle);
    DEINIT_HTTP_PARSER(_hp_parser);
    free(buf->base);
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
        WRN("Cannot accept new connection : gateway supports only one connection at once for now");
        return;
    }

    INF("New connection on http server");
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
    http_parser_settings_init(&_hp_settings);
    if(!_hp_parser)
        _hp_parser = calloc(1, sizeof(http_parser));
    _hp_settings.on_url = _url_asked_cb;
    http_parser_init(_hp_parser, HTTP_REQUEST);
}

/********************************
 *             API              *
 *******************************/

ZgInterfacesInterface *zg_http_server_init()
{
    if(_init_count != 0)
    {
        return NULL;
    }

    _log_domain = zg_logs_domain_register("zg_http_server", ZG_COLOR_GREEN);
    struct sockaddr_in bind_addr;

    if(uv_tcp_init(uv_default_loop(), &_server_handle) != 0)
    {
        ERR("Cannot initialize HTTP server handle");
        return NULL;
    }
    if(uv_ip4_addr(zg_conf_get_http_server_address(),zg_conf_get_http_server_port(), & bind_addr) !=0 ||
            uv_tcp_bind(&_server_handle, (struct sockaddr *)&bind_addr, 0) != 0)
    {
        ERR("Cannot bind HTTP server socket");
        return NULL;
    }

    if(uv_listen((uv_stream_t *)&_server_handle, MAX_PENDING_CONNECTTION, _new_connection_cb) != 0)
    {
        ERR("Cannot start listening for new connection");
    }
    memset(&_interface, 0, sizeof(_interface));
    sprintf((char *)_interface.name, "HTTP");

    INF("HTTP server started on address %s - port %d", zg_conf_get_http_server_address(), zg_conf_get_http_server_port());
    _init_count = 1;

    return &_interface;
}

void zg_http_server_shutdown()
{
    if(_init_count != 1)
    {
        return;
    }
    _init_count--;
    CLOSE_CLIENT(_client_handle);
    INF("HTTP_SERVER module shut down");
}
