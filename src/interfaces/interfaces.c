
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <Eina.h>
#include <time.h>

#include "interfaces.h"
#include "logs.h"
#include "utils.h"
#include "ipc.h"
#include "tcp.h"
#include "http_server.h"

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
 *        Local types           *
 *******************************/

typedef ZgInterfacesInterface*  (*submodule_init)       (void);
typedef void                    (*submodule_shutdown)   (void);

typedef struct
{
    submodule_init init;
    submodule_shutdown shutdown;
} SubmoduleAPI;


/********************************
 *      Local variables         *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;
static Eina_List *_interfaces = NULL;

typedef struct
{
    ZgInterfacesCommandId id;
    const char *string;
} CommandIdEntry;

static CommandIdEntry _command_id_table[] =
{
    {ZG_INTERFACES_COMMAND_GET_VERSION, "version"},
    {ZG_INTERFACES_COMMAND_OPEN_NETWORK, "open_network"},
    {ZG_INTERFACES_COMMAND_TOUCHLINK, "touchlink"},
    {ZG_INTERFACES_COMMAND_GET_DEVICE_LIST, "device_list"}
};

/* This table defines all enabled submodules */
/* This is the main entry ponit if you want to add a new interface */
static SubmoduleAPI _submodules[] = {
    {zg_ipc_init, zg_ipc_shutdown},
    {zg_tcp_init, zg_tcp_shutdown},
    {zg_http_server_init, zg_http_server_shutdown}
};

static int _nb_submodules = sizeof(_submodules)/sizeof(SubmoduleAPI);

/********************************
 *            Internal          *
 *******************************/

static ZgInterfacesCommandId _getcommandIdFromString(const char *string)
{
    int i = 0;
    int res = -1;

    for(i = 0; i < ZG_INTERFACES_COMMAND_MAX_ID; i++)
    {
        if(strncmp(string, _command_id_table[i].string, ZG_INTERFACES_MAX_COMMAND_STRING_LEN) == 0)
        {
            res = i;
            break;
        }
    }
    return res;
}

/********************************
 *       Answer functions       *
 *******************************/

static ZgInterfacesAnswerObject *_version_answer_get()
{
    ZgInterfacesAnswerObject *answer = NULL;
    CALLOC_ANSWER_RET_NULL(answer);
    answer->status = 0;
    answer->data = ANSWER_DATA_VERSION;
    answer->len = strlen(ANSWER_DATA_VERSION);
    return answer;
}

static ZgInterfacesAnswerObject *_error_answer_get()
{
    ZgInterfacesAnswerObject *answer = NULL;
    CALLOC_ANSWER_RET_NULL(answer);
    answer->status = 1;
    answer->data = ANSWER_DATA_ERROR;
    answer->len = strlen(ANSWER_DATA_ERROR);
    return answer;
}

/********************************
 *             API              *
 *******************************/

ZgInterfacesAnswerObject *zg_interfaces_process_command(ZgInterfacesInterface *interface, ZgInterfacesCommandObject *command)
{
    ZgInterfacesCommandId command_id;
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

    INF("Processing command %s", command->command_string);
    command_id = _getcommandIdFromString(command->command_string);

    /* Following the command ID, we use local functions to build and return an answer object. This object must be used
     * as payload for any considered interface */
    switch(command_id)
    {
        case ZG_INTERFACES_COMMAND_GET_VERSION:
            return _version_answer_get();
            break;
        default:
            return _error_answer_get();
            break;
    }

}

void zg_interfaces_free_command_object(ZgInterfacesCommandObject *obj)
{
    ZG_VAR_FREE(obj);
}


void zg_interfaces_free_answer_object(ZgInterfacesAnswerObject *obj)
{
    ZG_VAR_FREE(obj);
}

void zg_interfaces_init()
{
    ZgInterfacesInterface *buf = NULL;
    int i = 0;

    if(_init_count != 0)
        return;
    _log_domain = zg_logs_domain_register("zg_interfaces", ZG_COLOR_BLACK);
    _init_count++;

    /* Register all submodules */
    for(i = 0; i < _nb_submodules; i++)
    {
        buf = _submodules[i].init();
        if(buf)
        {
            _interfaces = eina_list_append(_interfaces, buf);
            INF("[%s] Interface added", buf->name);
        }
        else
        {
            ERR("Did not manage to properly initialize an interface");
        }
    }

    INF("Interfaces module started");
    return;
}

void zg_interfaces_shutdown()
{
    int i = 0;

    if(_init_count != 1)
        return;

    eina_list_free(_interfaces);
    for(i = 0; i < _nb_submodules; i++)
    {
        _submodules[i].shutdown();
    }

    _init_count--;
    INF("Interfaces module shut down");
}

void zg_interfaces_send_event(const char *event, json_t *data)
{
    json_t *root = NULL;
    uv_buf_t *buf = NULL;
    Eina_List *iterator;
    ZgInterfacesInterface *interface;
    size_t size;

    if(!data)
    {
        ERR("Cannot send event : data is empty");
        return;
    }

    INF("Sending event \"%s\" to remote clients", event);
    root = json_object();
    if(!root)
    {
        ERR("Cannot create root json to send event");
        return;
    }

    if(json_object_set_new(root, "type", json_string("event"))||
            json_object_set_new(root, "timestamp", json_integer(time(NULL))) ||
            json_object_set_new(data, "type", json_string(event)) ||
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
    EINA_LIST_FOREACH(_interfaces, iterator, interface)
    {
       if( interface && interface->event_cb)
       {
           DBG("[%s] Dispatch event", interface->name);
           interface->event_cb(buf);
       }
    }
    ZG_VAR_FREE(buf->base);
    ZG_VAR_FREE(buf);
}
