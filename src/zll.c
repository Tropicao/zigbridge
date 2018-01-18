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
#define SCAN_TIMEOUT                            0.25
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

static void _touchlink_finished(void)
{
    LOG_INF("Device reset, enabling security again");
    mt_sys_nv_write_enable_security(NULL);
}

static void _identify_delay_timeout_cb(uv_timer_t *t)
{
    uv_unref((uv_handle_t *)t);
    LOG_INF("Device identified itself, sending reset");
    zg_zll_send_factory_reset_request(_touchlink_finished);
}

static uint8_t _process_scan_response(void *data __attribute__((unused)), int len __attribute__((unused)))
{
    LOG_INF("A device is ready to install");
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
    mt_af_set_inter_pan_channel(ZLL_CHANNEL, cb);
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

static void _scan_timeout_cb(uv_timer_t *s __attribute__((unused)));

static void _scan_request_sent(void)
{
    if(_stop_scan !=1)
        uv_timer_start(&_scan_timeout_timer, _scan_timeout_cb, 250, 0);
}

static void _send_single_scan_request()
{
    zg_zll_send_scan_request(_scan_request_sent);
}

static void _scan_timeout_cb(uv_timer_t *s __attribute__((unused)))
{
    static int scan_counter = 0;

    scan_counter++;
    if(scan_counter < 5 && !_stop_scan)
    {
        _send_single_scan_request();
    }
    else
    {
        uv_unref((uv_handle_t *) &_scan_timeout_timer);
        if(!_stop_scan)
            mt_sys_nv_write_enable_security(NULL);
    }
}

static void _send_five_scan_requests()
{
    _stop_scan = 0;
    uv_timer_init(uv_default_loop(), &_scan_timeout_timer);
    _send_single_scan_request();
}

static void _security_disabled(void)
{
    _send_five_scan_requests();
}

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
    mt_sys_nv_write_disable_security(_security_disabled);
}
