#include <jansson.h>
#include <stdint.h>
#include <Eina.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logs.h"
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
    uint16_t cluster;
} ClusterData;

typedef struct
{
    uint8_t num;
    uint16_t profile;
    uint16_t device_id;
    uint8_t in_clusters_num;
    uint8_t out_clusters_num;
    Eina_List *in_clusters_list;
    Eina_List *out_clusters_list;
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
static int _log_domain = -1;

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

static void _destroy_cluster_data(ClusterData *cluster)
{
    ZG_VAR_FREE(cluster);
}

static void _destroy_endpoint_data(EndpointData *endpoint)
{
    int i = 0;
    if(endpoint)
    {
        for(i=0; i < endpoint->in_clusters_num; i++)
            _destroy_cluster_data(endpoint->in_clusters_list[i];
        for(i=0; i < endpoint->out_clusters_num; i++)
            _destroy_cluster_data(endpoint->out_clusters_list[i];
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

static EndpointData *_create_endpoint_data(  uint8_t num,
                                    uint16_t profile,
                                    uint8_t in_clusters_num,
                                    uint8_t out_clusters_num)
{
    EndpointData *result = calloc(1, sizeof(EndpointData));

    if(result)
    {
        result->num = num;
        result->profile = profile;
    }
    return result;
}


static void _add_endpoint(DeviceData *data, uint8_t num)
{
    EndpointData *endpoint = NULL;
    if(!data)
        return;

    if(_get_endpoint_by_num(data->endpoints, num) != NULL)
        return;

    endpoint = _create_endpoint_data(num, 0x0000, 0, 0);
    if(endpoint)
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
        ERR("Cannot add device to device list because data is empty");
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
    int i = 0;

    DBG(" =============== Device list ===============");
    EINA_LIST_FOREACH(_device_list, l, data)
    {
        DBG("[0x%02X] : Short : 0x%04X - Ext : 0x%"PRIx64,
                data->id, data->short_addr, data->ext_addr);
        EINA_LIST_FOREACH(data->endpoints, l_ep, endpoint)
        {
            DBG("Endpoint : 0x%02X, profile 0x%04X", endpoint->num, endpoint->profile);
            for(i=0; i < endpoint->in_clusters_num; i++)
                DBG("=== IN cluster     : 0x%04X", endpoint->in_clusters_list[i].cluster);
            for(i=0; i < endpoint->out_clusters_num; i++)
                DBG("=== OUT cluster    : 0x%04X", endpoint->out_clusters_list[i].cluster);
        }
    }

    DBG("============================================");

}

static void _load_cluster_data(EndpointData *data, json_t *cluster)
{
    ClusterData *cluster = NULL;

}
static void _load_endpoint_data(DeviceData *data, json_t *endpoint)
{
    EndpointData *ep = NULL;
    json_t *num = NULL, *profile = NULL, *device_id = NULL, *in_clusters_num = NULL, *out_clusters_num = NULL;
    int i = 0;

    if(!data || !endpoint || !json_is_object(endpoint))
    {
        ERR("Cannot load endpoint data from json file");
        return;
    }
    num = json_object_get(endpoint, "num");
    profile = json_object_get(endpoint, "profile");
    device_id = json_object_get(endpoint, "device_id");
    in_clusters_num = json_object_get(endpoint, "in_clusters_num");
    out_clusters_num = json_object_get(endpoint, "out_clusters_num");
    in
    if(!num || !json_is_integer(num) ||
            !profile || !json_is_integer(profile)||
            !device_id || !json_is_integer(device_id) ||
            !in_clusters_num || !json_is_integer(in_clusters_num) ||
            !out_clusters_num || !json_is_integer(out_clusters_num))
    {
        ERR("Cannot load enpoint data for device 0x%04X", data->short_addr);
    }
    else
    {
        ep = _create_endpoint_data(json_integer_value(num),
                json_integer_value(profile),
                json_integer_value(in_clusters_num),
                json_integer_value(out_clusters_num));
        data->endpoints = eina_list_append(data->endpoints, ep);

    }
    json_decref(num);
    json_decref(profile);
}

static void _load_device_data(json_t *device)
{
    /* Saved data */
    json_t *id = NULL;
    json_t *short_addr = NULL;
    json_t *ext_addr = NULL;
    json_t *endpoints;
    DeviceData *data = NULL;
    uint8_t ep_index;
    json_t *ep = NULL;

    if(!device || !json_is_object(device))
    {
        ERR("Cannot load endpoint data from json file");
        return;
    }

    id = json_object_get(device, "id");
    if(id)
    {
        short_addr = json_object_get(device, "short_addr");
        ext_addr = json_object_get(device, "ext_addr");

        data = _create_device_data( json_integer_value(id),
                json_integer_value(short_addr),
                json_integer_value(ext_addr));
        if(data)
        {
            _add_device_to_list(data);
            endpoints = json_object_get(device, "endpoints");
            if(endpoints && json_is_array(endpoints))
            {
                json_array_foreach(endpoints, ep_index, ep)
                {
                    _load_endpoint_data(data, ep);
                }
            }
        }

        else
            WRN("Cannot create device data from retrieved device data in file");

        json_decref(id);
        json_decref(short_addr);
        json_decref(ext_addr);
    }
    else
    {
        ERR("Cannot load device : id is not properly formatted");
    }

}

static void _load_device_list()
{
    json_t *root = NULL, *array = NULL;
    json_error_t error;
    const char *device_path = zg_conf_get_device_list_path();
    size_t index;
    json_t *value;

    eina_list_free(_device_list);
    root = json_load_file(zg_conf_get_device_list_path(), 0, &error);
    if(!root)
    {
        WRN("Cannot load devices list from file %s : [%s] l.%d c.%d : %s",
            device_path, error.source, error.line, error.column, error.text);
        return;
    }
    array = json_object_get(root, "devices");
    if(!array)
    {
        ERR("Cannot get device array from devices file");
        goto end_load_device;
    }

    json_array_foreach(array, index, value)
    {
        _load_device_data(value);
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

static json_t *_build_cluster_data_json(ClusterData *data)
{
    json_t *cluster = NULL;
    cluster = json_object();
    int i = 0;
    if(json_object_set_new(cluster, "cluster", json_integer(data->cluster)))
    {
        ERR("Cannot build json object for cluster 0x%02X", data->cluster);
    }
    return cluster;
}

static json_t *_build_endpoint_data_json(EndpointData *data)
{
    json_t *endpoint = NULL;
    int i = 0;
    json_t *in_clusters = NULL, out_clusters = NULL;
    json_t *cluster = NULL;

    in_clusters = json_array();
    for(i = 0; i < data->in_clusters_num; i++)
    {
        cluster = _build_cluster_data_json(data->in_clusters_list[i]);
        if(cluster)
        {
            json_array_append(in_clusters, cluster);
            json_decref(cluser);
        }
    }

    out_clusters = json_array();
    for(i = 0; i < data->out_clusters_num; i++)
    {
        cluster = _build_cluster_data_json(data->out_clusters_list[i]);
        if(cluster)
        {
            json_array_append(out_clusters, cluster);
            json_decref(cluser);
        }
    }

    endpoint = json_object();
    if(json_object_set_new(endpoint, "num", json_integer(data->num))||
            json_object_set_new(endpoint, "profile", json_integer(data->profile)) ||
            json_object_set_new(endpoint, "device_id", json_integer(data->device_id)) ||
            json_object_set_new(endpoint, "in_clusters_list", in_clusters) ||
            json_object_set_new(endpoint, "out_clusters_list", out_clusters))
    {
        ERR("Cannot build json object for endpoint 0x%02X", data->num);
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
        ERR("Cannot build json value to save device %d", data->id);
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
        INF("Stored data for device 0x%04X", data->short_addr);
    }

    root = json_object();
    json_object_set_new(root, "devices", array);
    if(json_dump_file(root, device_path, JSON_INDENT(DEVICES_NB_INDENT)))
        ERR("Cannot save device list in file %s", device_path);

    json_decref(root);
}

static json_t *_get_device_list_json(void)
{
    DeviceData *data = NULL;
    Eina_List *l = NULL;
    json_t *root = NULL, *array = NULL, *device = NULL;

    array = json_array();
    EINA_LIST_FOREACH(_device_list, l, data)
    {
        device = _build_device_data_json(data);
        if(device)
        {
            json_array_append(array, device);
            json_decref(device);
        }
        INF("Stored data for device 0x%04X", data->short_addr);
    }

    root = json_object();
    json_object_set_new(root, "devices", array);
    return root;
}

/********************************
 *             API              *
 *******************************/

void zg_device_init(uint8_t reset_device)
{
    eina_init();
    _log_domain = zg_logs_domain_register("zg_devices", ZG_COLOR_LIGHTGREEN);
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
        INF("Device already exists in device base, updating its data");
        data->short_addr = short_addr;
        _save_device_list();
        return -1;
    }

    tmp_id = _get_next_free_id();
    if(tmp_id < 0)
    {
        ERR("Cannot get free id, device database is full");
    }
    else
    {
        data = _create_device_data((DeviceId)tmp_id, short_addr, ext_addr);
        if(data)
        {
            INF("Saving new device with id %d", tmp_id);
            _add_device_to_list(data);
            _save_device_list();
        }
        else
        {
            ERR("Cannot create new device with id %d", tmp_id);
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

DeviceId zg_device_get_id(uint16_t short_addr)
{
    DeviceId id = 0xFF;
    Eina_List *l = NULL;
    DeviceData *data = NULL;

    EINA_LIST_FOREACH(_device_list, l, data)
    {
        if(data->short_addr == short_addr)
        {
            id = data->id;
            break;
        }
    }

    return id;
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
        ERR("Cannot save endpoints for device 0x%04X : device is unknown", short_addr);
        return;
    }
    if(nb_ep <= 0 || !ep_list)
    {
        ERR("Cannot save endpoints for device 0x%04X : %s.",
                short_addr, nb_ep > 0 ? "incorrect number of endpoints" : "endpoint list is NULL");
        return;
    }

    INF("Saving up-to-date endpoints list for device 0x%04X", short_addr);
    for(index = 0; index < nb_ep; index++)
    {
        _add_endpoint(data, ep_list[index]);
    }
    _save_device_list();
}

void zg_device_update_endpoint_data(uint16_t addr, ZgSimpleDescriptor *desc);
{
    EndpointData *ep = NULL;
    DeviceData *device = NULL;
    int i = 0;
    device = _get_device_by_short_addr(addr);
    if(device && desc)
    {
        ep = _get_endpoint_by_num(device->endpoints, desc->endpoint);
        if(ep)
        {
            ep->profile = profile;
            ep->device_id = device_id;
            ZG_VAR_FREE(ep->in_clusters_list);
            ZG_VAR_FREE(ep->out_clusters_list);
            ep->in_clusters_num = desc->in_clusters_num
            ep->out_clusters_num = desc->out_clusters_num
            ep->in_clusters_list = calloc(ep->in_clusters_num, sizeof(ClusterData));
            ep->out_clusters_list = calloc(ep->out_clusters_num, sizeof(ClusterData));
            for(i = 0; i < ep->in_clusters_num; i++)
                ep->in_clusters_list[i].cluster = desc->in_clusters_list[i]
            for(i = 0; i < ep->out_clusters_num; i++)
                ep->out_clusters_list[i].cluster = desc->out_clusters_list[i]
            _save_device_list();
        }
    }
}

uint8_t zg_device_get_next_empty_endpoint(uint16_t addr)
{
    uint8_t res = 0x00;
    Eina_List *l = NULL;
    EndpointData *endpoint = NULL;
    DeviceData *device = NULL;

    device = _get_device_by_short_addr(addr);
    if(device)
    {
        EINA_LIST_FOREACH(device->endpoints, l, endpoint)
        {
            if(!(endpoint->profile))
            {
                res = endpoint->num;
                break;
            }
        }
    }
    return res;
}

json_t *zg_device_get_device_list_json(void)
{
    return _get_device_list_json();
}




