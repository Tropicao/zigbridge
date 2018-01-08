#include <jansson.h>
#include <stdint.h>
#include <znp.h>
#include "device.h"
#include "list.h"
#include "utils.h"

/********************************
 *    Constants and macros      *
 *******************************/

/********************************
 *          Data types          *
 *******************************/

typedef struct
{
    DeviceId id;
    uint16_t short_addr;
} DeviceData;

/********************************
 *          Local variables     *
 *******************************/

static ZgList *_device_list = NULL;

/********************************
 *   Device data management     *
 *******************************/

static DeviceData *_create_device_data( DeviceId id,
                                        uint16_t short_addr) 
{
    DeviceData *result = calloc(1, sizeof(DeviceData));
    if(result)
    {
        result->id = id;
        result->short_addr = short_addr;
    }
    return result;
}

static void _destroy_device_data(DeviceData *data)
{
    ZG_VAR_FREE(data);
}

/********************************
 *   Device list management     *
 *******************************/

static int _get_next_free_id()
{
    DeviceId id = 0;
    uint8_t id_used = 0;
    ZgList *l;
    DeviceData *d;
    while(1)
    {
        ZG_LIST_FOREACH(_device_list, l, d)
        {
            if(d && d->id == id)
            {
                id_used = 1;
                break;
            }
        }
        if(!id_used)
            return id;
        else if(id == ZG_DEVICE_ID_MAX)
            return -1;
        else
            id++;
    }
}

static void _add_device_to_list(DeviceData *data)
{
    _device_list = zg_list_append(_device_list, data);
}

/********************************
 *          Internal            *
 *******************************/

void _load_device_list()
{

}

void _free_device_list()
{
    DeviceData *data = NULL;
    ZG_LIST_FREE(_device_list, data)
        _destroy_device_data(data);
}

void _save_device_list()
{
}

/********************************
 *             API              *
 *******************************/

void zg_device_init()
{
    _load_device_list();
}

void zg_device_shutdown()
{
    _save_device_list();
    _free_device_list();
}

DeviceId zg_add_device(uint16_t short_addr)
{
    int tmp_id = _get_next_free_id();
    DeviceData *data;
    if(tmp_id < 0)
    {
        LOG_ERR("Cannot get free id, device database is full");
    }
    else
    {
        LOG_INF("Saving new device with id %d", tmp_id);  
        data = _create_device_data((DeviceId)tmp_id, short_addr);
        _add_device_to_list(data);
    }
    return (DeviceId)tmp_id;
}
