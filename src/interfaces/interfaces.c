
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <Eina.h>

#include "interfaces.h"
#include "logs.h"

/********************************
 *          Constants           *
 *******************************/

#define CALLOC_ANSWER_RET_NULL(x)       do {\
                                            x = calloc(1, sizeof(ZgInterfacesAnswerObject));\
                                            if(!x)\
                                                return NULL;\
                                        } while(0)

#define ANSWER_DATA_VERSION             "{\"version\":\"0.4.0\"}"
#define ANSWER_DATA_ERROR               "{\"status\":\"Unknown command\"}"
#define ANSWER_DATA_OPEN_NETWORK_OK     "{\"open_network\":\"ok\"}"
#define ANSWER_DATA_TOUCHLINK_OK        "{\"touchlink\":\"ok\"}"
#define ANSWER_DATA_TOUCHLINK_KO        "{\"touchlink\":\"error\"}"
/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static Eina_List *_interfaces = NULL;

/********************************
 *            Internal          *
 *******************************/

static ZgInterfacesAnswerObject *_version_answer_get()
{
    ZgInterfacesAnswerObject *answer = NULL;
    CALLOC_ANSWER_RET_NULL(answer);
    answer->data = ANSWER_DATA_VERSION;
    answer->len = strlen(ANSWER_DATA_VERSION);
    return answer;
}

static ZgInterfacesAnswerObject *_error_answer_get()
{
    ZgInterfacesAnswerObject *answer = NULL;
    CALLOC_ANSWER_RET_NULL(answer);
    answer->data = ANSWER_DATA_ERROR;
    answer->len = strlen(ANSWER_DATA_ERROR);
    return answer;
}

//static void _allocated_req_sent(uv_write_t *req, int status __attribute__((unused)))
//{
//    uv_buf_t *buf = (uv_buf_t *)req->data;
//
//    free(buf->base);
//    free(buf);
//}
//
//static void _send_device_list()
//{
//    uv_write_t req;
//    size_t size;
//    json_t *devices = zg_device_get_device_list_json();
//
//    if(!_client)
//        return;
//
//    INF("Sending device list to remote client");
//    uv_buf_t *buf = calloc(1, sizeof(uv_buf_t));
//    size = json_dumpb(devices, NULL, 0, JSON_DECODE_ANY);
//    buf->base = calloc(size, sizeof(char));
//    size = json_dumpb(devices, buf->base, size, JSON_DECODE_ANY);
//    json_decref(devices);
//    buf->len = size;
//    req.data = buf;
//
//    uv_write(&req,(uv_stream_t *) _client, buf, 1, _allocated_req_sent);
//}
//
//static void _open_network(void)
//{
//    uv_write_t *req;
//    uv_buf_t buffer[] = {
//        {.base =INTERFACES_OPEN_NETWORK_OK, .len=21}
//    };
//
//    if(!_client)
//        return;
//    req = calloc(1, sizeof(uv_write_t));
//
//    INF("Sending Open Network OK to INTERFACES client");
//    uv_write(req, (uv_stream_t *)_client, buffer, 1, NULL);
//    zg_mt_zdo_permit_join(NULL);
//}
//
//static void _start_touchlink(void)
//{
//    uint8_t res;
//    uv_write_t *req;
//    uv_buf_t *buffer = NULL;
//    char *msg = NULL;
//
//    if(!_client)
//        return;
//
//    res = zg_zll_start_touchlink();
//    req = calloc(1, sizeof(uv_write_t));
//    buffer = calloc(1, sizeof(uv_buf_t));
//    msg = (res == 0 ? INTERFACES_TOUCHLINK_OK:INTERFACES_TOUCHLINK_KO);
//    buffer->base = calloc(strlen(msg)+1, sizeof(char));
//    buffer->len = strlen(msg) + 1 ;
//    strcpy(buffer->base, msg);
//    req->data = buffer;
//
//    INF("Sending touchlink status to INTERFACES client");
//    if(uv_write(req, (uv_stream_t *)_client, buffer, 1, _allocated_req_sent) <= 0)
//        ERR("Cannot send touchlink status to INTERFACES client");
//}

/********************************
 *             API              *
 *******************************/

void zg_interfaces_register_new_interface(ZgInterfacesInterface *interface)
{
    if(!interface)
    {
        ERR("Cannot register new interface : structure is NULL");
    }

    _interfaces = eina_list_append(_interfaces, interface);
    INF("[%s] Interface added", interface->name);
}

ZgInterfacesAnswerObject *zg_interfaces_process_command(ZgInterfacesInterface *interface, ZgInterfacesCommandObject *command)
{
    if(!interface)
    {
        ERR("Cannot process interface command : unknown interface");
        return NULL;
    }

    if(!command)
    {
        ERR("[%s] Cannot process command : command is empty", interface->name);
        return NULL;
    }

    switch(command->command)
    {
        case ZG_INTERFACES_COMMAND_GET_VERSION:
            return _version_answer_get();
            break;
        default:
            return _error_answer_get();
            break;
    }
}

void zg_interfaces_init()
{
    if(_init_count != 0)
        return;
    _log_domain = zg_logs_domain_register("zg_interfaces", ZG_COLOR_BLACK);
    _init_count++;
    INF("Interfaces module started");
    return;
}

void zg_interfaces_shutdown()
{
    if(_init_count != 1)
        return;
    eina_list_free(_interfaces);
    _init_count--;
    INF("Interfaces module shut down");
}
