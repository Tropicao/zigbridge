#include <znp.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zdp.h"
#include "aps.h"
#include "zcl.h"
#include "sm.h"
#include "mt_zdo.h"

/********************************
 *          Constants           *
 *******************************/

/********** Format **********/

/* Active Endpoint */
#define LEN_ACTIVE_ENDPOINT_REQ                 3
#define INDEX_TRANSACTION_SEQ_NUMBER            0
#define INDEX_NWK_ADDR_OF_INTEREST              1

/* Data */

/********** Application data **********/

/* App data */
#define ZDP_PROFILE_ID                          0x0000  /* ZDP */

/********************************
 *          Local variables     *
 *******************************/

static uint8_t _transaction_sequence_number = 0;
static SyncActionCb _init_complete_cb = NULL;
static ActiveEpRspCb _active_ep_cb = NULL;

/********************************
 *   ZDP messages callbacks     *
 *******************************/

static void _zdo_active_ep_rsp_cb(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list)
{
    if(_active_ep_cb)
        _active_ep_cb(short_addr, nb_ep, ep_list);
}

static void _zdp_message_cb(void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    LOG_DBG("Received ZDP data (%d bytes)", len);
    switch(buffer[2])
    {
        default:
            LOG_WARN("Unsupported ZDP command %02X", buffer[2]);
            break;
    }
    _transaction_sequence_number++;
}

/********************************
 * Initialization state machine *
 *******************************/

static ZgSm *_init_sm = NULL;

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
    {
        LOG_INF("ZDP application is initialized");
        if(_init_complete_cb)
        {
            zg_sm_destroy(_init_sm);
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

static ZgSmState _init_states[] = {
    {_register_zdp_endpoint, _general_init_cb},
};

static uint8_t _init_nb_states = sizeof(_init_states)/sizeof(ZgSmState);

/********************************
 *          ZLL API             *
 *******************************/

void zg_zdp_init(SyncActionCb cb)
{
    LOG_INF("Initializing ZDP");

    if(cb)
        _init_complete_cb = cb;

    mt_zdo_register_active_ep_rsp_callback(_zdo_active_ep_rsp_cb);
    _init_sm = zg_sm_create(_init_states, _init_nb_states);
    zg_sm_continue(_init_sm);
}

void zg_zdp_shutdown(void)
{
}

void zg_zdp_query_active_endpoints(uint16_t short_addr, SyncActionCb cb)
{
    char zdp_data[LEN_ACTIVE_ENDPOINT_REQ] = {0};

    LOG_INF("Sending active endpoint request");
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

void zg_zdp_register_active_endpoints_rsp(ActiveEpRspCb cb)
{
    _active_ep_cb = cb;
}
