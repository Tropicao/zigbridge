
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <Eina.h>

#include "interfaces.h"
#include "logs.h"
#include "utils.h"
#include "jansson.h"
#include "device.h"

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

static ZgInterfacesAnswerObject *_device_list_answer_get()
{
    ZgInterfacesAnswerObject *answer = NULL;
    json_t *list = zg_device_get_device_list_json();

    CALLOC_ANSWER_RET_NULL(answer);
    answer->status = 0;
    answer->data = json_dumps(list, JSON_DECODE_ANY);
    answer->len = strlen(answer->data);
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
        case ZG_INTERFACES_COMMAND_GET_DEVICE_LIST:
            return _device_list_answer_get();
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
