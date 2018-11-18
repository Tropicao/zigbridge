#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "http_server.h"
#include "core.h"
#include "logs.h"
#include "utils.h"
#include "conf.h"

/********************************
 *          Constants           *
 *******************************/

#define CLOSE_CLIENT(x)     do {\
                                if(_client_handle){\
                                uv_close((uv_handle_t *)x, NULL);\
                                ZG_VAR_FREE(x);\
                                }} while(0);

#define HTTP_SERVER_PIPENAME            "/tmp/zg_sock"
#define MAX_PENDING_CONNECTTION         2

/* STUB data */
#define STUB_ANSWER_STATUS              "HTTP/1.1 200 OK\r\n"
#define STUB_ANSWER_CONTENT_LENGTH      "Content-Length: 11\r\n"
#define STUB_ANSWER_CONTENT_TYPE        "Content-Type: text/html\r\n"
#define STUB_ANSWER_SPACER              "\r\n"
#define STUB_ANSWER_PAYLOAD             "Hello World\r\n"

/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static uv_tcp_t _server_handle;
static uv_tcp_t *_client_handle = NULL;

/********************************
 *            Internal          *
 *******************************/

static void _client_answer_cb(uv_write_t *req __attribute__((unused)), int status)
{
    if(status)
    {
        ERR("Client answer has failed");
    }
}

static void _stub_answer(void)
{
    uv_write_t req;
    uv_buf_t buf[] = {
        {.base = STUB_ANSWER_STATUS, .len = strlen(STUB_ANSWER_STATUS)},
        {.base = STUB_ANSWER_CONTENT_LENGTH, .len = strlen(STUB_ANSWER_CONTENT_LENGTH)},
        {.base = STUB_ANSWER_CONTENT_TYPE, .len = strlen(STUB_ANSWER_CONTENT_TYPE)},
        {.base = STUB_ANSWER_SPACER, .len = strlen(STUB_ANSWER_SPACER)},
        {.base = STUB_ANSWER_PAYLOAD, .len = strlen(STUB_ANSWER_PAYLOAD)}};

    INF("Answering stub HTTP response");
    req.data = buf[0].base;
    if(uv_write(&req, (uv_stream_t *)_client_handle, buf, sizeof(buf)/sizeof(uv_buf_t), _client_answer_cb) != 0)
    {
        ERR("Error writing to client");
    }
}

static void _process_http_server_data(char *data, int len)
{
   if(!data || len <= 0)
   {
      ERR("Cannot process HTTP_SERVER command : message is corrupted");
      CLOSE_CLIENT(_client_handle);
      return;
   }

   DBG("HTTP server data : %s", data);
   _stub_answer();
}

/****************************************
 *    HTTP_SERVER messages callbacks    *
 ****************************************/

static void _new_data_cb(uv_stream_t *s __attribute__((unused)), ssize_t n, const uv_buf_t *buf)
{
    if (n < 0)
    {
        if(n == UV_EOF)
            INF("Client disconnected from HTTP server");
        else
            ERR("User socket error (%zd)", n);
        CLOSE_CLIENT(_client_handle);
        return;
    }

    _process_http_server_data(buf->base, n);
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
}

/********************************
 *             API              *
 *******************************/

int zg_http_server_init()
{
    _log_domain = zg_logs_domain_register("zg_http_server", ZG_COLOR_GREEN);
    struct sockaddr_in bind_addr;

    if(_init_count == 1)
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

    INF("HTTP server started on address %s - port %d", zg_conf_get_http_server_address(), zg_conf_get_http_server_port());
    _init_count = 1;

    return 0;
}

void zg_http_server_shutdown()
{
    if(_init_count != 1)
    {
        return;
    }
    CLOSE_CLIENT(_client_handle);
    INF("HTTP_SERVER module shut down");
}
