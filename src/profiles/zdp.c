#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zdp.h"
#include "aps.h"
#include "zcl.h"
#include "action_list.h"
#include "mt_zdo.h"
#include "logs.h"

/********************************
 *          Constants           *
 *******************************/

/********** Format **********/

/* Active Endpoint */
#define LEN_ACTIVE_ENDPOINT_REQ                 3
#define INDEX_TRANSACTION_SEQ_NUMBER            0
#define INDEX_NWK_ADDR_OF_INTEREST              1

/* simple descriptor */
#define LEN_SIMPLE_DESC_REQ                     4
#define INDEX_ENDPOINT                          3

/* Data */

/********** Application data **********/

/* App data */
#define ZDP_PROFILE_ID                          0x0000  /* ZDP */

/********************************
 *          Local variables     *
 *******************************/

static int _log_domain = -1;
static uint8_t _transaction_sequence_number = 0;
static SyncActionCb _init_complete_cb = NULL;
static ActiveEpRspCb _active_ep_cb = NULL;
static SimpleDescRspCb _simple_desc_rsp_cb = NULL;

/********************************
 *   ZDP messages callbacks     *
 *******************************/

static void _zdo_active_ep_rsp_cb(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list)
{
    if(_active_ep_cb)
        _active_ep_cb(short_addr, nb_ep, ep_list);
}

static void _zdo_simple_desc_rsp_cb(uint8_t endpoint, uint16_t profile, uint16_t device_id)
{
    if(_simple_desc_rsp_cb)
        _simple_desc_rsp_cb(endpoint, profile, device_id);
}

static void _zdp_message_cb(uint16_t cluster __attribute__((unused)), void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    DBG("Received ZDP data (%d bytes)", len);
    switch(buffer[2])
    {
        default:
            WRN("Unsupported ZDP command %02X", buffer[2]);
            break;
    }
    _transaction_sequence_number++;
}

/********************************
 * Initialization state machine *
 *******************************/

static ZgAl *_init_sm = NULL;

static void _general_init_cb(void)
{
    if(zg_al_continue(_init_sm) != 0)
    {
        INF("ZDP application is initialized");
        if(_init_complete_cb)
        {
            zg_al_destroy(_init_sm);
            _init_complete_cb();
        }
    }
}

void _register_zdp_endpoint(SyncActionCb cb)
{
    zg_aps_register_endpoint(   ZCL_ZDP_ENDPOINT,
                                ZDP_PROFILE_ID,
                                0x0000,
                                0x00,
                                0x00,
                                NULL,
                                0x00,
                                NULL,
                                _zdp_message_cb,
                                cb);
}

static ZgAlState _init_states[] = {
    {_register_zdp_endpoint, _general_init_cb},
};

static uint8_t _init_nb_states = sizeof(_init_states)/sizeof(ZgAlState);

/********************************
 *          ZLL API             *
 *******************************/

void zg_zdp_init(SyncActionCb cb)
{
    _log_domain = zg_logs_domain_register("zg_zdp", ZG_COLOR_GREEN);
    INF("Initializing ZDP");

    if(cb)
        _init_complete_cb = cb;

    zg_mt_zdo_register_active_ep_rsp_callback(_zdo_active_ep_rsp_cb);
    zg_mt_zdo_register_simple_desc_rsp_cb(_zdo_simple_desc_rsp_cb);
    _init_sm = zg_al_create(_init_states, _init_nb_states);
    zg_al_continue(_init_sm);
}

void zg_zdp_shutdown(void)
{
}

void zg_zdp_query_active_endpoints(uint16_t short_addr, SyncActionCb cb)
{
    char zdp_data[LEN_ACTIVE_ENDPOINT_REQ] = {0};

    INF("Sending active endpoint request to device 0x%04X", short_addr);
    memcpy(zdp_data+INDEX_TRANSACTION_SEQ_NUMBER,
            &_transaction_sequence_number,
            sizeof(_transaction_sequence_number));
    memcpy(zdp_data+INDEX_NWK_ADDR_OF_INTEREST,
            &short_addr,
            sizeof(short_addr));

    zg_aps_send_data(short_addr,
            0xABCD,
            ZCL_ZDP_ENDPOINT,
            ZCL_ZDP_ENDPOINT,
            ZCL_CLUSTER_ACTIVE_ENDPOINTS_REQUEST,
            0x00,
            zdp_data,
            LEN_ACTIVE_ENDPOINT_REQ,
            cb);

}

void zg_zdp_query_simple_descriptor(uint16_t short_addr, uint8_t endpoint, SyncActionCb cb)
{
    char zdp_data[LEN_SIMPLE_DESC_REQ] = {0};

    INF("Sending simple descriptor request to device 0x%04X for endpoint 0x%02X", short_addr, endpoint);
    memcpy(zdp_data+INDEX_TRANSACTION_SEQ_NUMBER,
            &_transaction_sequence_number,
            sizeof(_transaction_sequence_number));
    memcpy(zdp_data+INDEX_NWK_ADDR_OF_INTEREST,
            &short_addr,
            sizeof(short_addr));
    zdp_data[INDEX_ENDPOINT] = endpoint;

    zg_aps_send_data(short_addr,
            0xABCD,
            ZCL_ZDP_ENDPOINT,
            ZCL_ZDP_ENDPOINT,
            ZCL_CLUSTER_SIMPLE_DESCRIPTOR_REQUEST,
            0x00,
            zdp_data,
            LEN_SIMPLE_DESC_REQ,
            cb);

}

void zg_zdp_register_active_endpoints_rsp(ActiveEpRspCb cb)
{
    _active_ep_cb = cb;
}

void zg_zdp_register_simple_desc_rsp(SimpleDescRspCb cb)
{
    _simple_desc_rsp_cb = cb;
}
