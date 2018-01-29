#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zll.h"
#include "aps.h"
#include "zcl.h"
#include "action_list.h"
#include "mt_af.h"
#include "mt_sys.h"
#include "sm.h"
#include "logs.h"
#include <jansson.h>
#include "ipc.h"

/********************************
 *          Constants           *
 *******************************/

#define ZLL_ENDPOINT                            0x2
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
#define ZLL_SCAN_TIMEOUT_MS                     250
#define ZLL_PROFIL_ID                           0xC05E  /* ZLL */
#define ZLL_DEVICE_ID                           0X0210  /* Extended color light */
#define ZLL_DEVICE_VERSION                      0x2     /* Version 2 */
#define ZLL_INFORMATION_FIELD                   0x12

#define ZLL_IDENTIFY_DELAY_MS                   3000

/********************************
 *          Local variables     *
 *******************************/

static int _log_domain = -1;
static uint16_t _zll_in_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_in_clusters_num = sizeof(_zll_in_clusters)/sizeof(uint8_t);

static uint16_t _zll_out_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_out_clusters_num = sizeof(_zll_out_clusters)/sizeof(uint8_t);

static uint32_t _interpan_transaction_identifier = 0;

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
 *         ZLL commands         *
 *******************************/

void _zll_send_scan_request(SyncActionCb cb)
{
    char zll_data[LEN_SCAN_REQUEST] = {0};

    INF("Sending Scan Request");
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

void _zll_send_identify_request(SyncActionCb cb)
{
    char zll_data[LEN_IDENTIFY_REQUEST] = {0};
    uint16_t identify_duration = DEFAULT_IDENTIFY_DURATION;

    INF("Sending identify request");
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

void _zll_send_factory_reset_request(SyncActionCb cb)
{
    char zll_data[LEN_FACTORY_RESET_REQUEST] = {0};

    INF("Sending factory reset request");
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


/********************************
 *    Touchlink state machine   *
 *******************************/

/*** Local variables ***/

static uv_timer_t _scan_timeout_timer;
static uv_timer_t _identify_timer;
static uint8_t _scan_count = 0;
static ZgSm *_touchlink_sm = NULL;


/*** States and events ***/

enum
{
    STATE_INIT,
    STATE_DISABLE_SECURITY,
    STATE_SEND_SCAN,
    STATE_WAIT_SCAN_RESPONSE,
    STATE_IDENTIY,
    STATE_WAIT_IDENTIFY,
    STATE_FACTORY_RESET,
    STATE_ENABLE_SECURITY,
    STATE_SHUTDOWN,
};

enum
{
    EVENT_INIT_DONE,
    EVENT_SECURITY_DISABLED,
    EVENT_SECURITY_ENABLED,
    EVENT_SCAN_SENT,
    EVENT_SCAN_TIMEOUT_RESCAN,
    EVENT_SCAN_TIMEOUT_NO_RESCAN,
    EVENT_SCAN_RESPONSE_RECEIVED,
    EVENT_IDENTIFY_REQUEST_SENT,
    EVENT_IDENTIFY_DELAY_PAST,
    EVENT_FACTORY_RESET_SENT
};

/*** States entry functions ***/

static void _init_touchlink(void)
{
    INF("Initializing touchlink state machine");
    _interpan_transaction_identifier = _generate_new_interpan_transaction_identifier();
    uv_timer_init(uv_default_loop(), &_scan_timeout_timer);
    uv_timer_init(uv_default_loop(), &_identify_timer);
    _scan_count = 0;
    zg_sm_send_event(_touchlink_sm, EVENT_INIT_DONE);
}

static void _send_ipc_event_touchlink_end()
{
    json_t *root;
    root = json_object();
    json_object_set_new(root, "status", json_string("finished"));

    zg_ipc_send_event(ZG_IPC_EVENT_TOUCHLINK_FINISHED, root);
    json_decref(root);
}

static void _shutdown_touchlink(void)
{
    uv_unref((uv_handle_t *)&_scan_timeout_timer);
    uv_unref((uv_handle_t *)&_identify_timer);
    zg_sm_destroy(_touchlink_sm);
    INF("Touchlink procedure finished");
    _send_ipc_event_touchlink_end();
}

static void _security_disabled_cb(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_SECURITY_DISABLED);
}

static void _disable_security(void)
{
    zg_mt_sys_nv_write_disable_security(_security_disabled_cb);
}

static void _security_enabled_cb(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_SECURITY_ENABLED);
}

static void _enable_security(void)
{
    zg_mt_sys_nv_write_enable_security(_security_enabled_cb);
}

static void _scan_request_sent(void)
{
    _scan_count++;
    zg_sm_send_event(_touchlink_sm, EVENT_SCAN_SENT);
}

static void _send_single_scan_request()
{
    _zll_send_scan_request(_scan_request_sent);
}

static void _scan_timeout_cb(uv_timer_t *s __attribute__((unused)))
{
    INF("Scan response timeout");
    if(_scan_count < 5)
        zg_sm_send_event(_touchlink_sm, EVENT_SCAN_TIMEOUT_RESCAN);
    else
        zg_sm_send_event(_touchlink_sm, EVENT_SCAN_TIMEOUT_NO_RESCAN);
}

static void _wait_scan_response(void)
{
    INF("Waiting for a scan response...");
    uv_timer_start(&_scan_timeout_timer, _scan_timeout_cb, ZLL_SCAN_TIMEOUT_MS, 0);
}

static void _identify_request_sent_cb(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_IDENTIFY_REQUEST_SENT);
}

static void _send_identify_request(void)
{
    _zll_send_identify_request(_identify_request_sent_cb);
}

static void _identify_delay_timeout_cb(uv_timer_t *s __attribute__((unused)))
{
    zg_sm_send_event(_touchlink_sm, EVENT_IDENTIFY_DELAY_PAST);
}

static void _wait_identify_period(void)
{
    uv_timer_start(&_identify_timer, _identify_delay_timeout_cb, ZLL_IDENTIFY_DELAY_MS, 0);
}

static void _factory_reset_sent_cb(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_FACTORY_RESET_SENT);
}

static void _send_factory_reset_request(void)
{
    _zll_send_factory_reset_request(_factory_reset_sent_cb);
}

static ZgSmStateData _touchlink_states[] = {
    {STATE_INIT, _init_touchlink},
    {STATE_DISABLE_SECURITY, _disable_security},
    {STATE_SEND_SCAN, _send_single_scan_request},
    {STATE_WAIT_SCAN_RESPONSE, _wait_scan_response},
    {STATE_IDENTIY, _send_identify_request},
    {STATE_WAIT_IDENTIFY, _wait_identify_period},
    {STATE_FACTORY_RESET, _send_factory_reset_request},
    {STATE_ENABLE_SECURITY, _enable_security},
    {STATE_SHUTDOWN, _shutdown_touchlink}
};
static ZgSmStateNb _touchlink_nb_states = sizeof(_touchlink_states)/sizeof(ZgSmStateData);

static ZgSmTransitionData _touchlink_transitions[] = {
    {STATE_INIT, EVENT_INIT_DONE, STATE_DISABLE_SECURITY},
    {STATE_DISABLE_SECURITY, EVENT_SECURITY_DISABLED, STATE_SEND_SCAN},
    {STATE_SEND_SCAN, EVENT_SCAN_SENT, STATE_WAIT_SCAN_RESPONSE},
    {STATE_WAIT_SCAN_RESPONSE, EVENT_SCAN_TIMEOUT_RESCAN, STATE_SEND_SCAN},
    {STATE_WAIT_SCAN_RESPONSE, EVENT_SCAN_RESPONSE_RECEIVED, STATE_IDENTIY},
    {STATE_WAIT_SCAN_RESPONSE, EVENT_SCAN_TIMEOUT_NO_RESCAN, STATE_ENABLE_SECURITY},
    {STATE_IDENTIY, EVENT_IDENTIFY_REQUEST_SENT, STATE_WAIT_IDENTIFY},
    {STATE_WAIT_IDENTIFY, EVENT_IDENTIFY_DELAY_PAST, STATE_FACTORY_RESET},
    {STATE_FACTORY_RESET, EVENT_FACTORY_RESET_SENT, STATE_ENABLE_SECURITY},
    {STATE_ENABLE_SECURITY, EVENT_SECURITY_ENABLED, STATE_SHUTDOWN}
};
static ZgSmTransitionNb _touchlink_nb_transtitions = sizeof(_touchlink_transitions)/sizeof(ZgSmTransitionData);



/********************************
 *   ZLL messages callbacks     *
 *******************************/

static uint8_t _process_scan_response(uint16_t short_addr __attribute__((unused)), void *data __attribute__((unused)), int len __attribute__((unused)))
{
    INF("A device has sent a scan response");
    zg_sm_send_event(_touchlink_sm, EVENT_SCAN_RESPONSE_RECEIVED);
    return 0;
}

static void _zll_message_cb(uint16_t short_addr, uint16_t cluster __attribute__((unused)), void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    DBG("Received ZLL data (%d bytes)", len);
    switch(buffer[2])
    {
        case COMMAND_SCAN_RESPONSE:
            INF("Received scan response");
            _process_scan_response(short_addr, buffer, len);
            break;
        default:
            WRN("Unsupported ZLL commissionning commande %02X", buffer[2]);
            break;
    }
}

/********************************
 * Initialization state machine *
 *******************************/

static ZgAl *_init_sm = NULL;

/* Callback triggered when ZLL initialization is complete */
static InitCompleteCb _init_complete_cb = NULL;

static void _general_init_cb(void)
{
    if(zg_al_continue(_init_sm) != 0)
    {
        INF("ZLL application is initialized");
        if(_init_complete_cb)
        {
            zg_al_destroy(_init_sm);
            _init_complete_cb();
        }
    }
}

static void _set_inter_pan_endpoint(SyncActionCb cb)
{
    zg_mt_af_set_inter_pan_endpoint(ZLL_ENDPOINT, cb);
}

static void _set_inter_pan_channel(SyncActionCb cb)
{
    zg_mt_af_set_inter_pan_channel(ZLL_CHANNEL, cb);
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

static ZgAlState _init_states[] = {
    {_register_zll_endpoint, _general_init_cb},
    {_set_inter_pan_endpoint, _general_init_cb},
    {_set_inter_pan_channel, _general_init_cb}
};
static int _init_nb_states = sizeof(_init_states)/sizeof(ZgAlState);

/********************************
 *          ZLL API             *
 *******************************/

void zg_zll_init(InitCompleteCb cb)
{
    _log_domain = zg_logs_domain_register("zg_zll", ZG_COLOR_LIGHTMAGENTA);
    INF("Initializing ZLL");
    if(cb)
        _init_complete_cb = cb;
    _init_sm = zg_al_create(_init_states, _init_nb_states);
    zg_al_continue(_init_sm);
}

void zg_zll_shutdown(void)
{
    zg_al_destroy(_init_sm);
}

uint8_t zg_zll_start_touchlink(void)
{
    if(_touchlink_sm)
    {
        WRN("A touchlink procedure is already in progress");
        return 1;
    }

    _touchlink_sm = zg_sm_create(   "touchlink",
                                    _touchlink_states,
                                    _touchlink_nb_states,
                                    _touchlink_transitions,
                                    _touchlink_nb_transtitions);
    if(!_touchlink_sm)
    {
        ERR("Abort touchlink procedure");
        return 1;
    }

    INF("Starting touchlink procedure");
    if(zg_sm_start(_touchlink_sm) != 0)
    {
        ERR("Error encountered while starting touchlink state machine");
        return 1;
    }
    return 0;
}
