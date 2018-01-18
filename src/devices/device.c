#include <jansson.h>
#include <stdint.h>
#include <znp.h>
#include <Eina.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "device.h"
#include "utils.h"
#include "conf.h"

/********************************
 *    Constants and macros      *
 *******************************/

#define DEVICES_NB_INDENT       4

/********************************
 *          Data types          *
 *******************************/

typedef struct
{
    uint8_t num;
} EndpointData;

typedef struct
{
    DeviceId id;
    uint16_t short_addr;
    uint64_t ext_addr;
    Eina_List *endpoints;
} DeviceData;

/********************************
 *          Local variables     *
 *******************************/

static Eina_List *_device_list = NULL;

/********************************
 *  Device data retrievement    *
 *******************************/

static DeviceData *_get_device_by_short_addr(uint16_t short_addr)
{
    Eina_List *l;
    DeviceData *data;

    EINA_LIST_FOREACH(_device_list, l, data)
    {
        if(data->short_addr == short_addr)
            break;
    }

    return data;
}


/********************************
 *  Endpoint data management    *
 *******************************/

static void _destroy_endpoint_data(EndpointData *endpoint)
{
    if(endpoint)
    {
    }
    ZG_VAR_FREE(endpoint);
}

static void _destroy_endpoints_list(DeviceData *data)
{
    EndpointData *endpoint = NULL;
    if(!data)
        return;

    EINA_LIST_FREE(data->endpoints, endpoint)
    {
        _destroy_endpoint_data(endpoint);
    }
    data->endpoints = NULL;
}

static EndpointData *_get_endpoint_by_num(Eina_List *ep, uint8_t num)
{
    EndpointData *res = NULL;
    Eina_List *l = NULL;
    if(!ep)
        return NULL;

    EINA_LIST_FOREACH(ep, l, res)
    {
        if(res->num == num)
            return res;
    }
    return res;
}



static void _add_endpoint(DeviceData *data, uint8_t num)
{
    EndpointData *endpoint = NULL;
    if(!data)
        return;

    if(_get_endpoint_by_num(data->endpoints, num) != NULL)
        return;

    endpoint = calloc(1, sizeof(EndpointData));
    if(!endpoint)
    {
        LOG_CRI("Error allocating memory for new endpoint");
        return;
    }

    endpoint->num = num;
    data->endpoints = eina_list_append(data->endpoints, endpoint);
}

/********************************
 *   Device data management     *
 *******************************/

static DeviceData *_create_device_data( DeviceId id,
                                        uint16_t short_addr,
                                        uint64_t ext_addr)
{
    DeviceData *result = calloc(1, sizeof(DeviceData));
    if(result)
    {
        result->id = id;
        result->short_addr = short_addr;
        result->ext_addr = ext_addr;
    }
    return result;
}

static void _destroy_device_data(DeviceData *data)
{
    if(data)
    {
        _destroy_endpoints_list(data);
    }

    ZG_VAR_FREE(data);
}

static DeviceData *_get_device_by_ext_addr(uint64_t ext_addr)
{
    Eina_List *l;
    DeviceData *data;

    EINA_LIST_FOREACH(_device_list, l, data)
    {
        if(data->ext_addr == ext_addr)
            break;
    }

    return data;
}



/********************************
 *   Device list management     *
 *******************************/

static int _get_next_free_id()
{
    DeviceId id = 0;
    uint8_t id_used = 1;
    Eina_List *l;
    DeviceData *d;
    while(id < ZG_DEVICE_ID_MAX && id_used)
    {
        id_used = 0;
        EINA_LIST_FOREACH(_device_list, l, d)
        {
            if(d && d->id == id)
            {
                id_used = 1;
                break;
            }
        }
        if(id_used)
            id++;
    }
    if(!id_used)
        return id;
    else
        return -1;
}

static void _add_device_to_list(DeviceData *data)
{
    if(!data)
    {
        LOG_ERR("Cannot add device to device list because data is empty");
        return;
    }
    _device_list = eina_list_append(_device_list, data);
}

/********************************
 *          Internal            *
 *******************************/

static void _print_device_list(void)
{
    Eina_List *l = NULL, *l_ep = NULL;
    DeviceData *data = NULL;
    EndpointData *endpoint = NULL;

    LOG_DBG(" =============== Device list ===============");
    EINA_LIST_FOREACH(_device_list, l, data)
    {
        LOG_DBG("[0x%02X] : Short : 0x%04X - Ext : 0x%016X",
                data->id, data->short_addr, data->ext_addr);
        EINA_LIST_FOREACH(data->endpoints, l_ep, endpoint)
            LOG_DBG("Endpoint : 0x%02X", endpoint->num);
    }

    LOG_DBG("============================================");

}

void _load_device_list()
{
    json_t *root = NULL, *array = NULL;
    json_error_t error;
    const char *device_path = zg_conf_get_device_list_path();
    size_t index, ep_index;
    json_t *value, *ep;
    DeviceData *data;

    /* Saved data */
    json_t *id = NULL;
    json_t *short_addr = NULL;
    json_t *ext_addr = NULL;
    json_t *endpoints;



    eina_list_free(_device_list);
    root = json_load_file(zg_conf_get_device_list_path(), 0, &error);
    if(!root)
    {
        LOG_WARN("Cannot load devices list from file %s : [%s] l.%d c.%d : %s",
            device_path, error.source, error.line, error.column, error.text);
        return;
    }
    array = json_object_get(root, "devices");
    if(!array)
    {
        LOG_ERR("Cannot get device array from devices file");
        goto end_load_device;
    }

    json_array_foreach(array, index, value)
    {
        id = json_object_get(value, "id");
        if(id)
        {
            short_addr = json_object_get(value, "short_addr");
            ext_addr = json_object_get(value, "ext_addr");

            data = _create_device_data( json_integer_value(id),
                                        json_integer_value(short_addr),
                                        json_integer_value(ext_addr));
            if(data)
            {
                _add_device_to_list(data);
                endpoints = json_object_get(value, "endpoints");
                if(endpoints && json_is_array(endpoints))
                {
                    json_array_foreach(endpoints, ep_index, ep)
                    {
                        _add_endpoint(data,json_integer_value(json_object_get(ep, "num")));
                    }
                }
            }

            else
                LOG_WARN("Cannot create device data from retrieved device data in file");

            json_decref(id);
            json_decref(short_addr);
            json_decref(ext_addr);
        }
        else
        {
            LOG_ERR("Cannot load device : id is not properly formatted");
        }
    }

    _print_device_list();

end_load_device:
    if(array)
        json_decref(array);
    if(root)
        json_decref(root);
}

void _free_device_list()
{
    DeviceData *data = NULL;
    EINA_LIST_FREE(_device_list, data)
        _destroy_device_data(data);
}

void _del_device_list(void)
{
    unlink(zg_conf_get_device_list_path());
}

static json_t *_build_endpoint_data_json(EndpointData *data)
{
    json_t *endpoint = NULL;
    endpoint = json_object();
    if(json_object_set_new(endpoint, "num", json_integer(data->num)))
    {
        LOG_ERR("Cannot build json object for endpoint 0x%02X", data->num);
    }
    return endpoint;
}

static json_t *_build_device_data_json(DeviceData *data)
{
    json_t *endpoints = NULL, *endpoint = NULL, *device = NULL;
    EndpointData *current_endpoint = NULL;
    Eina_List *l = NULL;

    endpoints = json_array();
    EINA_LIST_FOREACH(data->endpoints, l, current_endpoint)
    {
        endpoint = _build_endpoint_data_json(current_endpoint);
        if(endpoint)
        {
            json_array_append(endpoints, endpoint);
            json_decref(endpoint);
        }
    }
    device = json_object();

    if( json_object_set_new(device, "id", json_integer(data->id))                   ||
            json_object_set_new(device, "short_addr", json_integer(data->short_addr))   ||
            json_object_set_new(device, "ext_addr", json_integer(data->ext_addr))       ||
            json_object_set_new(device, "endpoints", endpoints ))
    {
        LOG_ERR("Cannot build json value to save device %d", data->id);
        json_decref(device);
        device = NULL;
    }
    return device;
}

static void _save_device_list()
{
    DeviceData *data = NULL;
    Eina_List *l = NULL;
    json_t *root = NULL, *array = NULL, *device = NULL;
    const char *device_path = zg_conf_get_device_list_path();

    array = json_array();
    EINA_LIST_FOREACH(_device_list, l, data)
    {
        device = _build_device_data_json(data);
        if(device)
        {
            json_array_append(array, device);
            json_decref(device);
        }
        LOG_INF("Stored data for device 0x%04X", data->short_addr);
    }

    root = json_object();
    json_object_set_new(root, "devices", array);
    if(json_dump_file(root, device_path, JSON_INDENT(DEVICES_NB_INDENT)))
        LOG_ERR("Cannot save device list in file %s", device_path);

    json_decref(root);
}

/********************************
 *             API              *
 *******************************/

void zg_device_init(uint8_t reset_device)
{
    eina_init();
    if(reset_device)
        _del_device_list();
    else
        _load_device_list();
}

void zg_device_shutdown()
{
    _free_device_list();
    eina_shutdown();
}

int zg_add_device(uint16_t short_addr, uint64_t ext_addr)
{
    int tmp_id = -1;
    DeviceData *data;

    data = _get_device_by_ext_addr(ext_addr);
    if(data)
    {
        LOG_INF("Device already exists in device base, updating its data");
        data->short_addr = short_addr;
        _save_device_list();
        return -1;
    }

    tmp_id = _get_next_free_id();
    if(tmp_id < 0)
    {
        LOG_ERR("Cannot get free id, device database is full");
    }
    else
    {
        data = _create_device_data((DeviceId)tmp_id, short_addr, ext_addr);
        if(data)
        {
            LOG_INF("Saving new device with id %d", tmp_id);
            _add_device_to_list(data);
            _save_device_list();
        }
        else
        {
            LOG_ERR("Cannot create new device with id %d", tmp_id);
            tmp_id = -1;
        }
    }
    return (DeviceId)tmp_id;
}

uint16_t zg_device_get_short_addr(DeviceId id)
{
    uint16_t addr = 0xFFFD;
    Eina_List *l = NULL;
    DeviceData *data = NULL;

    EINA_LIST_FOREACH(_device_list, l, data)
    {
        if(data->id == id)
        {
            addr = data->short_addr;
            break;
        }
    }

    return addr;
}

uint8_t zg_device_is_device_known(uint64_t ext_addr)
{
    DeviceData *data = _get_device_by_ext_addr(ext_addr);
    return (data != NULL);
}

void zg_device_update_endpoints(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list)
{
    DeviceData *data = _get_device_by_short_addr(short_addr);
    uint8_t index = 0;

    if(!data)
    {
        LOG_ERR("Cannot save endpoints for device 0x%04X : device is unknown", short_addr);
        return;
    }
    if(nb_ep <= 0 || !ep_list)
    {
        LOG_ERR("Cannot save endpoints for device 0x%04X : %s.",
                nb_ep > 0 ? "incorrect number of endpoints" : "endpoint list is NULL");
        return;
    }
    
    LOG_INF("Saving up-to-date endpoints list for device 0x%04X", short_addr);
    for(index = 0; index < nb_ep; index++)
    {
        _add_endpoint(data, ep_list[index]);
    }
    _save_device_list();
}

