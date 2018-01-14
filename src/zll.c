#include <znp.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zll.h"
#include "aps.h"
#include "zcl.h"
#include "sm.h"
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_util.h"
#include "mt_zdo.h"

/********************************
 *          Constants           *
 *******************************/

#define ZLL_ENDPOINT                            0x1
#define ZLL_CHANNEL                             0x0B

/********** Format **********/

/* ZLL command list */
#define COMMAND_SCAN_REQUEST                    0x00
#define COMMAND_SCAN_RESPONSE                   0x01
#define COMMAND_DEVICE_INFORMATION_REQUEST      0x02
#define COMMAND_IDENTIFY_REQUEST                0x06
#define COMMAND_FACTORY_NEW_REQUEST             0x07
#define COMMAND_NETWORK_START_REQUEST           0x10
#define COMMAND_NETWORK_JOIN_ROUTER_REQUEST     0x12
#define COMMAND_NETWORK_JOIN_END_DEVICE_REQUEST 0x14
#define COMMAND_NETWORK_UPDATE_REQUEST          0x16

/* Scan Request frame format */
#define LEN_SCAN_REQUEST                        6
#define INDEX_INTERPAN_TRANSACTION_IDENTIFIER   0
#define INDEX_ZIGBEE_INFORMATION                4
#define INDEX_ZLL_INFORMATION                   5

/* Scan Response frame format */
#define LEN_SCAN_RESPONSE                       29

/* Identify Request frame format */
#define LEN_IDENTIFY_REQUEST                    6
#define INDEX_IDENTIFY_DURATION                 4

/* Factory reset request frame format */
#define LEN_FACTORY_RESET_REQUEST               4

/********** Application data **********/

/* Scan request */
#define DEVICE_TYPE_COORDINATOR                 0x0
#define DEVICE_TYPE_ROUTER                      0x1
#define DEVICE_TYPE_END_DEVICE                  0x2

#define RX_ON_WHEN_IDLE                         (0x1<<2)
#define RX_OFF_WHEN_IDLE                        (0x0<<2)

/* Identify Request */
#define EXIT_IDENTIFY                           0x0000
#define DEFAULT_IDENTIFY_DURATION               0xFFFF


/* App data */
#define SCAN_RESPONSE_DELAY_MS                  250
#define ZLL_PROFIL_ID                           0xC05E  /* ZLL */
#define ZLL_DEVICE_ID                           0X0210  /* Extended color light */
#define ZLL_DEVICE_VERSION                      0x2     /* Version 2 */
#define ZLL_INFORMATION_FIELD                   0x12

/********************************
 *          Local variables     *
 *******************************/

static uv_timer_t _scan_timeout_timer;
static uv_timer_t _identify_timer;
static uint8_t _stop_scan = 0;
//static uint8_t _primary_channel = 11;
//static uint8_t _secondary_channels[] = {15, 20, 25};
//static uint8_t _nb_secondary_channels = sizeof(_secondary_channels)/sizeof(uint8_t);
static uint8_t _current_channel = ZLL_CHANNEL;

static uint32_t _interpan_transaction_identifier = 0;

static uint16_t _zll_in_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_in_clusters_num = sizeof(_zll_in_clusters)/sizeof(uint8_t);

static uint16_t _zll_out_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_out_clusters_num = sizeof(_zll_out_clusters)/sizeof(uint8_t);

/********************************
 *   ZLL messages callbacks     *
 *******************************/

static void _reset_sent(void)
{
    mt_sys_nv_write_enable_security(NULL);
}

static void _identify_delay_timeout_cb(uv_timer_t *t)
{
    uv_unref((uv_handle_t *)t);
    zg_zll_send_factory_reset_request(_reset_sent);
}

static uint8_t _process_scan_response(void *data __attribute__((unused)), int len __attribute__((unused)))
{
    LOG_INF("Received scan response from new device");
    _stop_scan = 1;
    zg_zll_send_identify_request(NULL);
    uv_timer_init(uv_default_loop(), &_identify_timer);
    uv_timer_start(&_identify_timer, _identify_delay_timeout_cb, 3000, 0);
    return 0;
}

static void _zll_message_cb(void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    LOG_DBG("Received ZLL data (%d bytes)", len);
    switch(buffer[2])
    {
        case COMMAND_SCAN_RESPONSE:
            LOG_INF("Received scan response");
            _process_scan_response(buffer, len);
            break;
        default:
            LOG_WARN("Unsupported ZLL commissionning commande %02X", buffer[2]);
            break;
    }
}

/********************************
 * Initialization state machine *
 *******************************/

static ZgSm *_init_sm = NULL;

/* Callback triggered when ZLL initialization is complete */
static InitCompleteCb _init_complete_cb = NULL;

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
    {
        LOG_INF("ZLL application is initialized");
        if(_init_complete_cb)
        {
            zg_sm_destroy(_init_sm);
            _init_complete_cb();
        }
    }
}

static void _set_inter_pan_endpoint(SyncActionCb cb)
{
    mt_af_set_inter_pan_endpoint(ZLL_ENDPOINT, cb);
}

static void _set_inter_pan_channel(SyncActionCb cb)
{
    mt_af_set_inter_pan_channel(_current_channel, cb);
}

void _register_zll_endpoint(SyncActionCb cb)
{
    zg_aps_register_endpoint(   ZLL_ENDPOINT,
                                ZLL_PROFIL_ID,
                                ZLL_DEVICE_ID,
                                ZLL_DEVICE_VERSION,
                                _zll_in_clusters_num,
                                _zll_in_clusters,
                                _zll_out_clusters_num,
                                _zll_out_clusters,
                                _zll_message_cb,
                                cb);
}



static ZgSmState _init_states[] = {
    {_register_zll_endpoint, _general_init_cb},
    {_set_inter_pan_endpoint, _general_init_cb},
    {_set_inter_pan_channel, _general_init_cb}
};
static int _init_nb_states = sizeof(_init_states)/sizeof(ZgSmState);

/********************************
 *    Touchlink state machine   *
 *******************************/

static ZgSm *_touchlink_sm = NULL;

static void _send_single_scan_request(SyncActionCb cb)
{
    LOG_INF("Sending ZLL scan request");
    zg_zll_send_scan_request(cb);
}

static void _wait_scan_response(SyncActionCb cb)
{
    LOG_INF("Waiting for scan response for %d ms", SCAN_RESPONSE_DELAY_MS);
    uv_timer_cb timer_cb = (uv_timer_cb)cb;
    uv_timer_start(&_scan_timeout_timer, timer_cb, SCAN_RESPONSE_DELAY_MS, 0);
}

/*
static void _switch_to_primary_channel(SyncActionCb cb)
{
    LOG_INF("Switching to ZLL primary channel (%d)", _primary_channel);
    mt_sys_nv_write_channel(_primary_channel, cb);
}

static void _switch_to_new_secondary_channel(SyncActionCb cb)
{
    static uint8_t index = 0;
    if(index < _nb_secondary_channels)
    {
        LOG_INF("Switching to ZLL secondary channel (%d)", _secondary_channels[index]);
        mt_sys_nv_write_channel(_secondary_channels[index], cb);
        index++;
    }
    else
        cb();
}
*/
static void _touchlink_callback(void)
{
    if(_stop_scan || zg_sm_continue(_touchlink_sm) != 0)
    {
        if(!_stop_scan)
            LOG_INF("Touchlink procedure ended without finding any new device");
        _stop_scan = 0;
        uv_unref((uv_handle_t *) &_scan_timeout_timer);
        zg_sm_destroy(_init_sm);
    }
}

static ZgSmState _touchlink_states[] = {
    {mt_sys_nv_write_disable_security, _touchlink_callback},
    {_send_single_scan_request, _touchlink_callback},
    {_wait_scan_response, _touchlink_callback},
    {_send_single_scan_request, _touchlink_callback},
    {_wait_scan_response, _touchlink_callback},
    {_send_single_scan_request, _touchlink_callback},
    {_wait_scan_response, _touchlink_callback},
    {_send_single_scan_request, _touchlink_callback},
    {_wait_scan_response, _touchlink_callback},
};
static int _touchlink_nb_states = sizeof(_touchlink_states)/sizeof(ZgSmState);
/********************************
 *          Internal            *
 *******************************/

uint32_t _generate_new_interpan_transaction_identifier()
{
    uint32_t result = 0;
    result = rand() & 0xFF;
    result |= (rand() & 0xFF) << 8;
    result |= (rand() & 0xFF) << 16;
    result |= (rand() & 0xFF) << 24;

    return result;
}

/********************************
 *          ZLL API             *
 *******************************/

void zg_zll_init(InitCompleteCb cb)
{
    LOG_INF("Initializing ZLL");
    if(cb)
        _init_complete_cb = cb;
    _init_sm = zg_sm_create(_init_states, _init_nb_states);
    zg_sm_continue(_init_sm);
}

void zg_zll_shutdown(void)
{
    _stop_scan = 1;
    zg_sm_destroy(_init_sm);
}

void zg_zll_send_scan_request(SyncActionCb cb)
{
    char zll_data[LEN_SCAN_REQUEST] = {0};

    memcpy(zll_data+INDEX_INTERPAN_TRANSACTION_IDENTIFIER,
            &_interpan_transaction_identifier,
            sizeof(_interpan_transaction_identifier));
    zll_data[INDEX_ZIGBEE_INFORMATION] = DEVICE_TYPE_ROUTER | RX_ON_WHEN_IDLE;
    zll_data[INDEX_ZLL_INFORMATION] = ZLL_INFORMATION_FIELD ;

    zg_aps_send_data(ZCL_BROADCAST_SHORT_ADDR,
            ZCL_BROADCAST_INTER_PAN,
            ZLL_ENDPOINT,
            ZCL_BROADCAST_ENDPOINT,
            ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
            COMMAND_SCAN_REQUEST,
            zll_data,
            LEN_SCAN_REQUEST,
            cb);
}

void zg_zll_send_identify_request(SyncActionCb cb)
{
    char zll_data[LEN_IDENTIFY_REQUEST] = {0};
    uint16_t identify_duration = DEFAULT_IDENTIFY_DURATION;

    LOG_INF("Sending identify request");
    memcpy(zll_data+INDEX_INTERPAN_TRANSACTION_IDENTIFIER,
            &_interpan_transaction_identifier,
            sizeof(_interpan_transaction_identifier));
    memcpy(zll_data + INDEX_IDENTIFY_DURATION, &identify_duration, 2);

    zg_aps_send_data(ZCL_BROADCAST_SHORT_ADDR,
            ZCL_BROADCAST_INTER_PAN,
            ZLL_ENDPOINT,
            ZCL_BROADCAST_ENDPOINT,
            ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
            COMMAND_IDENTIFY_REQUEST,
            zll_data,
            LEN_IDENTIFY_REQUEST,
            cb);
}

void zg_zll_send_factory_reset_request(SyncActionCb cb)
{
    char zll_data[LEN_FACTORY_RESET_REQUEST] = {0};

    LOG_INF("Sending factory reset request");
    memcpy(zll_data+INDEX_INTERPAN_TRANSACTION_IDENTIFIER,
            &_interpan_transaction_identifier,
            sizeof(_interpan_transaction_identifier));

    zg_aps_send_data(ZCL_BROADCAST_SHORT_ADDR,
            ZCL_BROADCAST_INTER_PAN,
            ZLL_ENDPOINT,
            ZCL_BROADCAST_ENDPOINT,
            ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
            COMMAND_FACTORY_NEW_REQUEST,
            zll_data,
            LEN_FACTORY_RESET_REQUEST,
            cb);

}

void zg_zll_start_touchlink(void)
{
    LOG_INF("Starting touchlink procedure");
    _interpan_transaction_identifier = _generate_new_interpan_transaction_identifier();
    _stop_scan = 0;
    uv_timer_init(uv_default_loop(), &_scan_timeout_timer);
    _touchlink_sm = zg_sm_create(_touchlink_states, _touchlink_nb_states);
    zg_sm_continue(_touchlink_sm);
}
