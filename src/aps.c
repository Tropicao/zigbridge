#include <stdlib.h>
#include <string.h>
#include <znp.h>
#include "aps.h"
#include "mt_af.h"
#include "utils.h"
#include "zcl.h"

/********************************
 *          Data types          *
 *******************************/

typedef struct ApsEndpoint
{
    uint8_t endpoint;
    ApsMsgCb cb;
    struct ApsEndpoint *next;
} ApsEndpoint;


/********************************
 *          Constants           *
 *******************************/

/* APS Header format */
#define ZCL_HEADER_SIZE             3
#define INDEX_FCS                   0x0
#define INDEX_TRANS_SEQ_NUM         0x1
#define INDEX_COMMAND               0x2

#define APS_DEFAULT_FRAME_CONTROL   0x11

/* APS incoming message format */
#define INDEX_GROUP_ID              0
#define INDEX_CLUSTER_ID            2
#define INDEX_SRC_ADDR_MODE         4
#define INDEX_SRC_ADDR              5
#define INDEX_SRC_ENDPOINT          13
#define INDEX_SRC_PAN_ID            14
#define INDEX_DST_ENDPOINT          16
#define INDEX_WAS_BROADCAST         17
#define INDEX_LINK_QUALITY          18
#define INDEX_SECURITY_USE          19
#define INDEX_TIMESTAMP             20
#define INDEX_TRANS_SEQUENCE_NUM    24
#define INDEX_LEN                   25
#define INDEX_DATA                  26

/********************************
 *          Local variables     *
 *******************************/

static ApsEndpoint *_endpoints_list = NULL;
static uint8_t _transaction_sequence_number = 0;
static SyncActionCb _current_cb = NULL;

/********************************
 *          Internal            *
 *******************************/

static ApsEndpoint *_find_endpoint(uint8_t endpoint)
{
    ApsEndpoint *result = _endpoints_list;
    while (result != NULL && result->endpoint != endpoint)
        result = result->next;

    return result;
}

static ApsEndpoint *_add_new_endpoint(uint8_t endpoint, ApsMsgCb cb)
{
    ApsEndpoint *buffer = _find_endpoint(endpoint);
    ApsEndpoint *tail = NULL;

    if(buffer)
    {
        LOG_ERR("Required endpoint is already registered");
        return NULL;
    }

    buffer = calloc(1, sizeof(ApsEndpoint));
    if(!buffer)
    {
        LOG_CRI("Cannot allocate memory to register new endpoint");
        return NULL;
    }
    buffer->endpoint = endpoint;
    buffer->cb = cb;

    if(!_endpoints_list)
    {
        _endpoints_list = buffer;
    }
    else
    {
        tail = _endpoints_list;
        while(tail->next != NULL)
            tail = tail->next;
        tail->next = buffer;
    }
    return _endpoints_list;
}

static void _clear_endpoint_list()
{
    ApsEndpoint *buffer = _endpoints_list;
    while(_endpoints_list)
    {
        _endpoints_list = _endpoints_list->next;
        ZG_VAR_FREE(buffer);
        buffer = _endpoints_list;
    }
}

static void _af_data_request_ext_cb(void)
{
    _transaction_sequence_number++;
    if(_current_cb)
    {
        _current_cb();
        _current_cb = NULL;
    }
}

uint8_t _build_frame_control()
{
    return APS_DEFAULT_FRAME_CONTROL;
}

/************************************
 *     APS msg callbacks            *
 ***********************************/

static void _process_aps_msg(uint16_t addr, uint8_t endpoint_num, uint16_t cluster, void *data, int len)
{
    ApsEndpoint *endpoint = NULL;
    uint8_t *aps_data = data;
    if(!aps_data || len <= 0)
    {
        LOG_WARN("Received empty APS message");
        return;
    }
    endpoint = _find_endpoint(endpoint_num);
    if(endpoint)
        endpoint->cb(addr, cluster, aps_data, len);
    else
        LOG_WARN("Received message is not for one of registered endpoint (0x%02X)", endpoint_num);
}

/************************************
 *          APS API                 *
 ***********************************/

void zg_aps_init()
{
    mt_af_register_incoming_message_callback(_process_aps_msg);
}

void zg_aps_shutdown()
{
    _clear_endpoint_list();
}

void zg_aps_register_endpoint(  uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                ApsMsgCb msg_cb,
                                SyncActionCb cb)
{
    if(!_add_new_endpoint(endpoint, msg_cb))
        return;

    /* We do not need to register ZDP endpoint (0x0000) since it is enabled by default */
    if(endpoint == ZCL_ZDP_ENDPOINT)
    {
        if(cb)
            cb();
        return;
    }

    mt_af_register_endpoint(endpoint,
            profile,
            device_id,
            device_ver,
            in_clusters_num,
            in_clusters_list,
            out_clusters_num,
            out_clusters_list,
            cb);
}

void zg_aps_send_data(  uint16_t dst_addr,
                        uint16_t dst_pan,
                        uint8_t src_endpoint,
                        uint8_t dst_endpoint,
                        uint16_t cluster,
                        uint8_t command,
                        void *data,
                        int len,
                        SyncActionCb cb)
{
    uint8_t *aps_data = NULL;
    int aps_data_len = len;

    if(src_endpoint != ZCL_ZDP_ENDPOINT)
        aps_data_len += ZCL_HEADER_SIZE;

    aps_data = calloc(aps_data_len, sizeof(uint8_t));
    if(!aps_data)
    {
        LOG_CRI("Cannot allocate memory for AF request");
        return;
    }

    _current_cb = cb;

    if(src_endpoint != ZCL_ZDP_ENDPOINT)
    {
        aps_data[INDEX_FCS] = _build_frame_control();
        aps_data[INDEX_TRANS_SEQ_NUM] = _transaction_sequence_number;
        aps_data[INDEX_COMMAND] = command;
        if(len > 0 && data)
            memcpy(aps_data + ZCL_HEADER_SIZE, data, len);
    }
    else
    {
        memcpy(aps_data, data, len);
    }

    mt_af_send_data_request_ext(dst_addr,
            dst_pan,
            src_endpoint,
            dst_endpoint,
            cluster,
            aps_data_len,
            aps_data,
            _af_data_request_ext_cb);
    free(aps_data);
}

