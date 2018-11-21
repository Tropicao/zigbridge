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

#define CLOSE_CLIENT(x)                 do {\
                                            if(_client_handle){\
                                                uv_read_stop((uv_stream_t *)x);\
                                                uv_close((uv_handle_t *)x, NULL);\
                                                ZG_VAR_FREE(x);\
                                            }} while(0);

#define DEINIT_HTTP_PARSER(x)           do {\
                                            ZG_VAR_FREE(x);\
                                        } while(0);
#define HTTP_SERVER_PIPENAME            "/tmp/zg_sock"
#define MAX_PENDING_CONNECTTION         2

#define MAX_URL_SIZE                    512

/* STUB data */
#define STUB_ANSWER_STATUS              "HTTP/1.1 200 OK\r\n"
#define STUB_ANSWER_CONTENT_LENGTH      "Content-Length: 11\r\n"
#define STUB_ANSWER_CONTENT_TYPE        "Content-Type: text/html\r\n"
#define STUB_ANSWER_SPACER              "\r\n"
#define STUB_ANSWER_PAYLOAD             "Hello World\r\n"
static uv_buf_t stub_buf[] = {
    {.base = STUB_ANSWER_STATUS, .len = strlen(STUB_ANSWER_STATUS)},
    {.base = STUB_ANSWER_CONTENT_LENGTH, .len = strlen(STUB_ANSWER_CONTENT_LENGTH)},
    {.base = STUB_ANSWER_CONTENT_TYPE, .len = strlen(STUB_ANSWER_CONTENT_TYPE)},
    {.base = STUB_ANSWER_SPACER, .len = strlen(STUB_ANSWER_SPACER)},
    {.base = STUB_ANSWER_PAYLOAD, .len = strlen(STUB_ANSWER_PAYLOAD)}};

/* Parser data */
#define REST_URL_VERSION            "/version"

/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static uv_tcp_t _server_handle;
static uv_tcp_t *_client_handle = NULL;
http_parser_settings _hp_settings;
http_parser *_hp_parser = NULL;
ZgInterfacesInterface _interface;

/********************************
 *            Internal          *
 *******************************/

static void _client_answer_cb(uv_write_t *req, int status)
{
    if(status)
    {
        ERR("Client answer has failed");
    }
    ZG_VAR_FREE(req);
}

static void _send_answer(void *data, int len)
{
    uv_write_t *req = calloc(1, sizeof(uv_write_t));

    if(uv_write(req, (uv_stream_t *)_client_handle, stub_buf, sizeof(stub_buf)/sizeof(uv_buf_t), _client_answer_cb) != 0)
    {
        ERR("Error writing to client");
    }
}

static int _url_asked_cb(http_parser *_hp __attribute__((unused)), const char *at __attribute__((unused)), size_t len __attribute__((unused)))
{

    struct http_parser_url url;
    int i = 0;

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

    for(i = 0; i < UF_MAX; i++)
    {
        if(url.field_set & (1 << i))
        {
            DBG("Found URL component (index %d, size %d): %.*s", i, url.field_data[i].len, url.field_data[i].len, at + url.field_data[i].off);
            if(strncmp(at + url.field_data[i].off, REST_URL_VERSION, strlen(REST_URL_VERSION)) == 0)
                
        }
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
new_data_end_error:
    CLOSE_CLIENT(_client_handle);
    DEINIT_HTTP_PARSER(_hp_parser);
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

int zg_http_server_init()
{
    zg_interfaces_init();
    _log_domain = zg_logs_domain_register("zg_http_server", ZG_COLOR_GREEN);
    struct sockaddr_in bind_addr;

    if(_init_count != 0)
    {
        return 0;
    }

    if(uv_tcp_init(uv_default_loop(), &_server_handle) != 0)
    {
        ERR("Cannot initialize HTTP server handle");
        return 1;
    }
    if(uv_ip4_addr(zg_conf_get_http_server_address(),zg_conf_get_http_server_port(), & bind_addr) !=0 ||
            uv_tcp_bind(&_server_handle, (struct sockaddr *)&bind_addr, 0) != 0)
    {
        ERR("Cannot bind HTTP server socket");
        return 1;
    }

    if(uv_listen((uv_stream_t *)&_server_handle, MAX_PENDING_CONNECTTION, _new_connection_cb) != 0)
    {
        ERR("Cannot start listening for new connection");
    }
    memset(&_interface, 0, sizeof(_interface));
    sprintf((char *)_interface.name, "HTTP");
    zg_interfaces_register_new_interface(&_interface);

    INF("HTTP server started on address %s - port %d", zg_conf_get_http_server_address(), zg_conf_get_http_server_port());
    _init_count = 1;

    return 0;
}

void zg_http_server_shutdown()
{
    if(--_init_count != 1)
    {
        return;
    }
    _init_count--;
    CLOSE_CLIENT(_client_handle);
    zg_interfaces_shutdown();
    INF("HTTP_SERVER module shut down");
}
