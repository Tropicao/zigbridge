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
#include "utils.h"
#include "crypto.h"
#include "keys.h"

/********************************
 *          Constants           *
 *******************************/

#define ZLL_ENDPOINT                            0x2
#define ZLL_CHANNEL                             0x0B
#define ZLL_PAN_ID                              0xABCE

/********** Format **********/

/* ZLL command list */
#define COMMAND_SCAN_REQUEST                        0x00
#define COMMAND_SCAN_RESPONSE                       0x01
#define COMMAND_DEVICE_INFORMATION_REQUEST          0x02
#define COMMAND_IDENTIFY_REQUEST                    0x06
#define COMMAND_FACTORY_NEW_REQUEST                 0x07
#define COMMAND_NETWORK_START_REQUEST               0x10
#define COMMAND_NETWORK_JOIN_ROUTER_REQUEST         0x12
#define COMMAND_NETWORK_JOIN_ROUTER_RESPONSE        0x13
#define COMMAND_NETWORK_JOIN_END_DEVICE_REQUEST     0x14
#define COMMAND_NETWORK_JOIN_END_DEVICE_RESPONSE    0x15
#define COMMAND_NETWORK_UPDATE_REQUEST              0x16

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

/* Network join end device request frame format */
#define LEN_END_DEVICE_JOIN_REQUEST             47

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
static int _init_count = 0;
static uint16_t _zll_in_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_in_clusters_num = sizeof(_zll_in_clusters)/sizeof(uint8_t);

static uint16_t _zll_out_clusters[] = {
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING};
static uint8_t _zll_out_clusters_num = sizeof(_zll_out_clusters)/sizeof(uint8_t);

static uint32_t _interpan_transaction_identifier = 0;
static uint32_t _touchlink_response_identifier = 0;

static void (*_new_device_cb)(uint16_t short_addr, uint64_t ext_addr) = NULL;
static uint64_t _demo_ext_addr = 0x00178801011514C4;

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
            ADDR_MODE_16_BITS,
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
            ADDR_MODE_16_BITS,
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
            ADDR_MODE_16_BITS,
            ZCL_BROADCAST_INTER_PAN,
            ZLL_ENDPOINT,
            ZCL_BROADCAST_ENDPOINT,
            ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
            COMMAND_FACTORY_NEW_REQUEST,
            zll_data,
            LEN_FACTORY_RESET_REQUEST,
            cb);

}

static void _zll_send_end_device_join_request(SyncActionCb cb)
{
    char zll_data[LEN_END_DEVICE_JOIN_REQUEST] = {0};
    uint64_t extended_pan_identifier = 0x2211FFEEDDCCBBAA;
    uint64_t dest_addr = _demo_ext_addr;
    uint8_t key_index = 4;
    uint8_t nwk_key[16] = {0};
    uint8_t nwk_update_identifier = 0x01; /* TBD */
    uint8_t logical_channel = 0xB;
    uint16_t pan_id = ZLL_PAN_ID;
    uint16_t nwk_addr = 0x22;
    uint8_t index = 0;

    zg_keys_get_encrypted_network_key_for_zll(_interpan_transaction_identifier, _touchlink_response_identifier, nwk_key);
    INF("Sending End Device Join request");

    index = 0;
    memcpy(zll_data + index, &_interpan_transaction_identifier, sizeof(_interpan_transaction_identifier));
    index += sizeof(_interpan_transaction_identifier);
    memcpy(zll_data + index, &extended_pan_identifier, sizeof(extended_pan_identifier));
    index += sizeof(extended_pan_identifier);
    memcpy(zll_data + index, &key_index, sizeof(key_index));
    index += sizeof(key_index);
    memcpy(zll_data + index, &nwk_key, sizeof(nwk_key));
    index += sizeof(nwk_key);
    memcpy(zll_data + index, &nwk_update_identifier, sizeof(nwk_update_identifier));
    index += sizeof(nwk_update_identifier);
    memcpy(zll_data + index, &logical_channel, sizeof(logical_channel));
    index += sizeof(logical_channel);
    memcpy(zll_data + index, &pan_id, sizeof(pan_id));
    index += sizeof(pan_id);
    memcpy(zll_data + index, &nwk_addr, sizeof(nwk_addr));
    index += sizeof(nwk_addr);

    DBG("About to send encrypted key :");
    for(index = 0; index < 16; index++)
        DBG("[%d] 0x%02X", index, nwk_key[index]);

    zg_aps_send_data(dest_addr,
            ADDR_MODE_64_BITS,
            ZCL_BROADCAST_INTER_PAN,
            ZLL_ENDPOINT,
            ZCL_BROADCAST_ENDPOINT,
            ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
            COMMAND_NETWORK_JOIN_ROUTER_REQUEST,
            zll_data,
            LEN_END_DEVICE_JOIN_REQUEST,
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
    EVENT_FACTORY_RESET_SENT,
    EVENT_JOIN_END_DEVICE_REQUEST_SENT,
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
    _touchlink_sm = NULL;
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
    uv_timer_stop(&_scan_timeout_timer);
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

/*
static void _factory_reset_sent_cb(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_FACTORY_RESET_SENT);
}
*/

static void _end_device_join_request_sent(void)
{
    zg_sm_send_event(_touchlink_sm, EVENT_JOIN_END_DEVICE_REQUEST_SENT);
}

static void _send_factory_reset_request(void)
{
    _zll_send_end_device_join_request(_end_device_join_request_sent);
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
    {STATE_FACTORY_RESET, EVENT_JOIN_END_DEVICE_REQUEST_SENT, STATE_ENABLE_SECURITY},
    {STATE_ENABLE_SECURITY, EVENT_SECURITY_ENABLED, STATE_SHUTDOWN}
};
static ZgSmTransitionNb _touchlink_nb_transtitions = sizeof(_touchlink_transitions)/sizeof(ZgSmTransitionData);



/********************************
 *   ZLL messages callbacks     *
 *******************************/

static uint8_t _process_scan_response(uint16_t short_addr __attribute__((unused)), void *data, int len __attribute__((unused)))
{
    INF("A device has sent a scan response");
    memcpy(&_touchlink_response_identifier, data + 3 + 9, sizeof(_touchlink_response_identifier));
    DBG("Transaction identifier : 0x%08X", _interpan_transaction_identifier);
    DBG("Response identifier : 0x%08X", _touchlink_response_identifier);
    zg_sm_send_event(_touchlink_sm, EVENT_SCAN_RESPONSE_RECEIVED);
    return 0;
}

static uint8_t _process_join_end_device_response(uint16_t short_addr __attribute__((unused)), void *data, int len __attribute__((unused)))
{
    uint8_t *payload = data;

    if(payload[7] != 0)
        WRN("Error making device to join the network (%d)", payload[7]);
    else
        INF("End device 0x%04X has joined the network", short_addr);

    return 0;
}

static uint8_t _process_join_router_response(uint16_t short_addr __attribute__((unused)), void *data, int len __attribute__((unused)))
{
    uint8_t *payload = data;
    INF("A router has sent a end device join response");

    if(payload[7] != 0)
        WRN("Error making device to join the network (%d)", payload[7]);
    else
        INF("Router 0x%04X has joined the network", 0x22);

    if(_new_device_cb)
    {
        INF("Starting learning device");
        _new_device_cb(0x22, _demo_ext_addr);
    }

    return 0;
}

static void _process_touchlink_commissioning_command(uint16_t short_addr, uint8_t *data, int len)
{
    int i;
    for (i = 0; i< len; i++)
        INF("Data %d : 0x%02X", i, data[i]);
    switch(data[2])
    {
        case COMMAND_SCAN_RESPONSE:
            _process_scan_response(short_addr, data, len);
            break;
        case COMMAND_NETWORK_JOIN_END_DEVICE_RESPONSE:
            _process_join_end_device_response(short_addr, data, len);
            break;
        case COMMAND_NETWORK_JOIN_ROUTER_RESPONSE:
            _process_join_router_response(short_addr, data, len);
            break;
        default:
            WRN("Unsupported ZLL touchlink commissioning command 0x%02X", data[2]);
            break;
    }
}

static void _zll_message_cb(uint16_t short_addr, uint16_t cluster, void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    DBG("Received ZLL data (%d bytes)", len);
    switch(cluster)
    {
        case ZCL_CLUSTER_TOUCHLINK_COMMISSIONING:
            _process_touchlink_commissioning_command(short_addr, data, len);
            break;
        default:
            WRN("Unsupported ZLL cluster 0x%04X", cluster);
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
            _init_sm = NULL;
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

uint8_t zg_zll_init(InitCompleteCb cb)
{
    ENSURE_SINGLE_INIT(_init_count);
    zg_aps_init();
    _log_domain = zg_logs_domain_register("zg_zll", ZG_COLOR_LIGHTMAGENTA);
    INF("Initializing ZLL");
    if(cb)
        _init_complete_cb = cb;
    _init_sm = zg_al_create(_init_states, _init_nb_states);
    zg_al_continue(_init_sm);
    return 0;
}

void zg_zll_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    zg_aps_shutdown();
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

void zg_zll_register_device_ind_callback(void (*cb)(uint16_t short_addr, uint64_t ext_addr))
{
    _new_device_cb = cb;
}
