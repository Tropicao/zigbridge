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
    DeviceId id;
    uint16_t short_addr;
    uint64_t ext_addr;
} DeviceData;

/********************************
 *          Local variables     *
 *******************************/

static Eina_List *_device_list = NULL;

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
    Eina_List *l = NULL;
    DeviceData *data = NULL;

    LOG_DBG(" =============== Device list ===============");
    EINA_LIST_FOREACH(_device_list, l, data)
        LOG_DBG("[0x%02X] : Short : 0x%04X - Ext : 0x%016X");
    LOG_DBG("============================================");

}

void _load_device_list()
{
    json_t *root = NULL, *array = NULL;
    json_error_t error;
    const char *device_path = zg_conf_get_device_list_path();
    size_t index;
    json_t *value;
    DeviceData *data;

    /* Saved data */
    json_t *id = NULL;
    json_t *short_addr = NULL;
    json_t *ext_addr = NULL;



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
                _add_device_to_list(data);
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

void _save_device_list()
{
    Eina_List *l = NULL;
    DeviceData *data = NULL;
    json_t *root = NULL, *array = NULL, *device = NULL;
    const char *device_path = zg_conf_get_device_list_path();

    array = json_array();
    EINA_LIST_FOREACH(_device_list, l, data)
    {
        device = json_object();
        if( json_object_set_new(device, "id", json_integer(data->id))                   ||
            json_object_set_new(device, "short_addr", json_integer(data->short_addr))   ||
            json_object_set_new(device, "ext_addr", json_integer(data->ext_addr))        )
        {
            LOG_ERR("Cannot build json value to save device %d", data->id);
            json_decref(root);
            return;
        }
        json_array_append_new(array, device);
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

void zg_device_init()
{
    eina_init();
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
        LOG_ERR("Device already exists in device base");
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
