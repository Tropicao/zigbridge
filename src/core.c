#include <stdlib.h>
#include "core.h"
#include "zdp.h"
#include "zha.h"
#include "zll.h"
#include "action_list.h"
#include "aps.h"
#include "mt.h"
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_zdo.h"
#include "mt_util.h"
#include "stdin.h"
#include "ipc.h"
#include "device.h"
#include "keys.h"
#include "sm.h"
#include "logs.h"

/********************************
 *     Constants and macros     *
 *******************************/

#define DEMO_DEVICE_ID              0
#define GATEWAY_ADDR                0x0000
#define GATEWAY_CHANNEL             11
#define GATEWAY_PAN_ID              0xABCD
#define GATEWAY_EXT_PAN_ID          0x2211FFEEDDCCBBAA

/*** For demo purpose, pre-calculated values */
#define X_RED       65535
#define Y_RED       0
#define X_BLUE      0
#define Y_BLUE      0

/********************************
 *       Local variables        *
 *******************************/

static int _log_domain = -1;

/********************************
 * Initialization state machine *
 *******************************/

static ZgAl *_init_sm = NULL;
static uint8_t _initialized = 0;
static uint8_t _reset_network = 0;

/* Callback triggered when core initialization is complete */

static void _general_init_cb(void)
{
    if(zg_al_continue(_init_sm) != 0)
    {
        INF("=======================================");
        INF("=== Core application is initialized ===");
        INF("=======================================");
        zg_keys_test_nwk_key_encryption_zll();
        _initialized = 1;
    }
}

static void _write_clear_flag(SyncActionCb cb)
{
    if(_reset_network)
        zg_mt_sys_nv_write_startup_options(STARTUP_CLEAR_STATE|STARTUP_CLEAR_CONFIG,cb);
    else
        cb();
}

static void _set_pan_id(SyncActionCb cb)
{
    zg_mt_sys_nv_set_pan_id(GATEWAY_PAN_ID, cb);
}

static void _set_ext_pan_id(SyncActionCb cb)
{
    uint64_t ext_pan_id = GATEWAY_EXT_PAN_ID;
    zg_mt_sys_nv_set_ext_pan_id((uint8_t *)&ext_pan_id, cb);
}

static void _write_channel(SyncActionCb cb)
{
    zg_mt_sys_nv_write_channel(GATEWAY_CHANNEL, cb);
}

static void _announce_gateway(SyncActionCb cb)
{
    zg_mt_zdo_device_annce(GATEWAY_ADDR, zg_mt_sys_get_ext_addr(), cb);
}


static void _get_demo_device_route(SyncActionCb cb)
{
    zg_mt_zdo_ext_route_disc_request(zg_device_get_short_addr(DEMO_DEVICE_ID), cb);
}

static void _zll_init(SyncActionCb cb)
{
    if(zg_zll_init(cb) != 0)
        ERR("Error initializing ZLL profile");
}
static void _zha_init(SyncActionCb cb)
{
    if(zg_zha_init(cb) != 0)
        ERR("Error initializing ZHA profile");
}

static void _zdp_init(SyncActionCb cb)
{
    if(zg_zdp_init(cb) != 0)
        ERR("Error initializing ZDP profile");
}


static ZgAlState _init_states_reset[] = {
    {_write_clear_flag, _general_init_cb},
    {zg_mt_sys_reset_dongle, _general_init_cb},
    {zg_mt_sys_nv_write_nwk_key, _general_init_cb},
    {_set_ext_pan_id, _general_init_cb},
    {zg_mt_sys_reset_dongle, _general_init_cb},
    {zg_mt_sys_check_ext_addr, _general_init_cb},
    {zg_mt_sys_nv_write_coord_flag, _general_init_cb},
    {_set_pan_id, _general_init_cb},
    {_write_channel, _general_init_cb},
    {zg_mt_sys_ping, _general_init_cb},
    {zg_mt_util_af_subscribe_cmd, _general_init_cb},
    {_zll_init, _general_init_cb},
    {_zha_init, _general_init_cb},
    {_zdp_init, _general_init_cb},
    {zg_mt_zdo_startup_from_app, _general_init_cb},
    {zg_mt_sys_nv_write_enable_security, _general_init_cb},
    {_announce_gateway, _general_init_cb},
};

static int _init_reset_nb_states = sizeof(_init_states_reset)/sizeof(ZgAlState);

static ZgAlState _init_states_restart[] = {
    {zg_mt_sys_reset_dongle, _general_init_cb},
    {zg_mt_sys_check_ext_addr, _general_init_cb},
    {zg_mt_sys_ping, _general_init_cb},
    {zg_mt_util_af_subscribe_cmd, _general_init_cb},
    {_zll_init, _general_init_cb},
    {_zha_init, _general_init_cb},
    {_zdp_init, _general_init_cb},
    {zg_mt_zdo_startup_from_app, _general_init_cb},
    {zg_mt_sys_nv_write_enable_security, _general_init_cb},
    {_get_demo_device_route, _general_init_cb},
    {_announce_gateway, _general_init_cb}
};
static int _init_restart_nb_states = sizeof(_init_states_restart)/sizeof(ZgAlState);

/********************************
 *  New device learning SM      *
 *******************************/

/*** Local variables ***/
static uint16_t _current_learning_device_addr = 0xFFFD;
ZgSm *_new_device_sm = NULL;

enum
{
    STATE_INIT,
    STATE_ACTIVE_ENDPOINT_DISC,
    STATE_SIMPLE_DESC_DISC,
    STATE_SHUTDOWN,
};

enum
{
    EVENT_INIT_DONE,
    EVENT_ACTIVE_ENDPOINTS_RESP,
    EVENT_SIMPLE_DESC_RECEIVED,
    EVENT_ALL_SIMPLE_DESC_RECEIVED,
};

static void _init_new_device_sm(void)
{
    zg_sm_send_event(_new_device_sm, EVENT_INIT_DONE);
}

static void _query_active_endpoints(void)
{
    zg_zdp_query_active_endpoints(_current_learning_device_addr, NULL);
}

static void _query_simple_desc(void)
{
    uint8_t endpoint = zg_device_get_next_empty_endpoint(_current_learning_device_addr);
    zg_zdp_query_simple_descriptor(_current_learning_device_addr, endpoint, NULL);
}

static void _shutdown_new_device_sm(void)
{
    INF("Learning device process finished");
    _current_learning_device_addr = 0xFFFD;
    zg_sm_destroy(_new_device_sm);
}

static ZgSmStateData _new_device_states[] = {
    {STATE_INIT, _init_new_device_sm},
    {STATE_ACTIVE_ENDPOINT_DISC, _query_active_endpoints},
    {STATE_SIMPLE_DESC_DISC, _query_simple_desc},
    {STATE_SHUTDOWN, _shutdown_new_device_sm}
};
static ZgSmStateNb _new_device_nb_states = sizeof(_new_device_states)/sizeof(ZgSmStateData);

static ZgSmTransitionData _new_device_transitions[] = {
    {STATE_INIT, EVENT_INIT_DONE, STATE_ACTIVE_ENDPOINT_DISC},
    {STATE_ACTIVE_ENDPOINT_DISC, EVENT_ACTIVE_ENDPOINTS_RESP, STATE_SIMPLE_DESC_DISC},
    {STATE_SIMPLE_DESC_DISC, EVENT_SIMPLE_DESC_RECEIVED, STATE_SIMPLE_DESC_DISC},
    {STATE_SIMPLE_DESC_DISC, EVENT_ALL_SIMPLE_DESC_RECEIVED, STATE_SHUTDOWN},
};
static ZgSmTransitionNb _new_device_nb_transitions = sizeof(_new_device_transitions)/sizeof(ZgSmTransitionData);

/********************************
 *  Network Events processing   *
 *******************************/

static void _send_ipc_event_new_device(DeviceId id)
{
    json_t *root = NULL;

    root = json_object();
    if(root)
        json_object_set_new(root, "id", json_integer(id));
    else
        return;

    zg_ipc_send_event(ZG_IPC_EVENT_NEW_DEVICE, root);
    json_decref(root);
}

static void _new_device_cb(uint16_t short_addr, uint64_t ext_addr)
{
    DeviceId id = 0;
    if(!zg_device_is_device_known(ext_addr))
    {
        INF("Seen device is a new device");
        id = zg_add_device(short_addr, ext_addr);
        _send_ipc_event_new_device(id);
        if(_new_device_sm)
        {
            WRN("Already learning a new device, cannot learn newly visible device");
            return;
        }
        _new_device_sm = zg_sm_create(  "New device",
                                        _new_device_states,
                                        _new_device_nb_states,
                                        _new_device_transitions,
                                        _new_device_nb_transitions);
        if(!_new_device_sm)
        {
            ERR("Abort new device learning");
            return;
        }
        INF("Start learning new device properties");
        _current_learning_device_addr = short_addr;
        zg_sm_start(_new_device_sm);
    }
    else
    {
        INF("Visible device is already learnt");
    }
}

static void _active_endpoints_cb(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list)
{
    uint8_t index = 0;
    INF("Device 0x%04X has %d active endpoints :", short_addr, nb_ep);
    for(index = 0; index < nb_ep; index++)
    {
        INF("Active endpoint 0x%02X", ep_list[index]);
    }
    zg_device_update_endpoints(short_addr, nb_ep, ep_list);
    zg_sm_send_event(_new_device_sm, EVENT_ACTIVE_ENDPOINTS_RESP);
}

static void _simple_desc_cb(uint8_t endpoint, uint16_t profile, uint16_t device_id)
{
    uint8_t next_endpoint;
    INF("Endpoint 0x%02X of device 0x%04X has profile 0x%04X",
            endpoint, _current_learning_device_addr, profile);
    zg_device_update_endpoint_data(_current_learning_device_addr, endpoint, profile, device_id);

    next_endpoint = zg_device_get_next_empty_endpoint(_current_learning_device_addr);
    if(next_endpoint)
        zg_sm_send_event(_new_device_sm, EVENT_SIMPLE_DESC_RECEIVED);
    else
        zg_sm_send_event(_new_device_sm, EVENT_ALL_SIMPLE_DESC_RECEIVED);
}

/********************************
 *     Commands processing      *
 *******************************/


static void _send_ipc_event_button_state_change(DeviceId id, uint8_t state)
{
    json_t *root;
    root = json_object();
    json_object_set_new(root, "id", json_integer(id));
    json_object_set_new(root, "state", json_integer(state));

    zg_ipc_send_event(ZG_IPC_EVENT_BUTTON_STATE, root);
    json_decref(root);
}


static void _button_change_cb(uint16_t addr, uint8_t state)
{
    DeviceId id = 0;

    INF("Button pressed, toggling the light");
    if(_initialized)
    {
        id = zg_device_get_id(addr);
        _send_ipc_event_button_state_change(id, state);
        if(addr != 0xFFFD && state == 0)
            zg_zha_switch_bulb_state(zg_device_get_short_addr(DEMO_DEVICE_ID));
        else
            WRN("Device is not installed, cannot switch light");
    }
    else
    {
        WRN("Core application has not finished initializing, cannot switch bulb state");
    }
}

static void _compute_new_color(uint16_t temp, uint16_t *x, uint16_t *y)
{
    /*
     * Simple demo function : we want to move hue from blue to red when
     * temperature increase.
     * We will agree that first read temperature (ie room temperature) is the
     * cold one (ie hue is blue), and that red hue can be reached at 30°C (can
     * be reached by blowing hot air on sensor)
     *
     * TLDR :
     * T=Cold : X=0, Y=0
     * T=Hot : X=65535, Y=0
     */
    static float a, b;
    static uint16_t ref_temp = 0;
    const uint16_t max_temp = 2700;


    if(temp > max_temp)
        temp = max_temp;

    if(ref_temp == 0 || temp < ref_temp)
    {
        DBG("Setting new temperature reference (%s)", ref_temp ? "New temp below ref temp":"No ref temp yet");
        ref_temp = temp;
        *y = 0;
        *x = 0;
        a = (float)(65535/(float)(max_temp - ref_temp));
        b = -(float)(a*ref_temp);
    }
    else
    {
        DBG("Updating temperature color");
        *y=0;
        *x= a*temp + b;
    }
    DBG("X : 0x%04X - Y : 0x%04X", *x, *y);
}


static void _send_ipc_event_temperature(DeviceId id, uint16_t temp)
{
    json_t *root;
    root = json_object();
    json_object_set_new(root, "id", json_integer(id));
    json_object_set_new(root, "temperature", json_integer(temp));

    zg_ipc_send_event(ZG_IPC_EVENT_TEMPERATURE, root);
    json_decref(root);
}

static void _temperature_cb(uint16_t addr, uint16_t temp)
{
    DeviceId id = 0xFF;
    uint16_t x, y;

    INF("New temperature report (%.2f°C), adjusting the light",(float)(temp/100.0));
    if(_initialized)
    {
        id = zg_device_get_id(addr);
        _send_ipc_event_temperature(id, temp);
        if(addr != 0xFFFD)
        {
            _compute_new_color(temp, &x, &y);
            zg_zha_move_to_color(zg_device_get_short_addr(DEMO_DEVICE_ID), x, y, 5);
        }
        else
            WRN("Device is not installed, cannot switch light");
    }
    else
    {
        WRN("Core application has not finished initializing, cannot switch bulb state");
    }
}



static void _process_command_touchlink()
{
    if(_initialized)
        zg_zll_start_touchlink();
    else
        WRN("Core application has not finished initializing, cannot start touchlink");
}

static void _process_command_switch_light()
{
    uint16_t addr = 0xFFFD;
    if(_initialized)
    {
        addr = zg_device_get_short_addr(DEMO_DEVICE_ID);
        if(addr != 0xFFFD)
            zg_zha_switch_bulb_state(addr);
        else
            WRN("Device is not installed, cannot switch light");
    }
    else
    {
        WRN("Core application has not finished initializing, cannot switch bulb state");
    }

}

static void _process_command_move_predefined_color(uint16_t x, uint16_t y)
{
    uint16_t addr = 0xFFFD;
    if(_initialized)
    {
        addr = zg_device_get_short_addr(DEMO_DEVICE_ID);
        if(addr != 0xFFFD)
            zg_zha_move_to_color(addr, x, y, 1);
        else
            WRN("Device is not installed, cannot switch light to predefined color");
    }
    else
    {
        WRN("Core application has not finished initializing, cannot switch bulb state");
    }
}

static void _process_command_open_network(void)
{
    INF("Opening network to allow new devices to join");
    zg_mt_zdo_permit_join(NULL);
}

static void _process_user_command(StdinCommand cmd)
{
    switch(cmd)
    {
        case ZG_STDIN_COMMAND_TOUCHLINK:
            _process_command_touchlink();
            break;
        case ZG_STDIN_COMMAND_SWITCH_DEMO_LIGHT:
            _process_command_switch_light();
            break;
        case ZG_STDIN_COMMAND_OPEN_NETWORK:
            _process_command_open_network();
            break;
        case ZG_STDIN_COMMAND_MOVE_TO_BLUE:
            _process_command_move_predefined_color(X_BLUE, Y_BLUE);
            break;
        case ZG_STDIN_COMMAND_MOVE_TO_RED:
            _process_command_move_predefined_color(X_RED, Y_RED);
            break;
        default:
            WRN("Unsupported command");
            break;
    }
}


/********************************
 *             API              *
 *******************************/

void zg_core_init(uint8_t reset_network)
{
    _log_domain = zg_logs_domain_register("zg_core", ZG_COLOR_BLACK);
    INF("Initializing core application");
    _reset_network = reset_network;
    _reset_network |= !zg_keys_check_network_key_exists();
    if(_reset_network)
        zg_keys_network_key_del();
    zg_stdin_register_command_cb(_process_user_command);
    zg_zha_register_device_ind_callback(_new_device_cb);
    zg_zll_register_device_ind_callback(_new_device_cb);
    zg_zha_register_button_state_cb(_button_change_cb);
    zg_zha_register_temperature_cb(_temperature_cb);
    zg_zdp_register_active_endpoints_rsp(_active_endpoints_cb);
    zg_zdp_register_simple_desc_rsp(_simple_desc_cb);
    zg_mt_init();
    zg_ipc_init();
    zg_keys_init();
    zg_device_init(_reset_network);

    if(_reset_network)
        _init_sm = zg_al_create(_init_states_reset, _init_reset_nb_states);
    else
        _init_sm = zg_al_create(_init_states_restart, _init_restart_nb_states);
    zg_al_continue(_init_sm);
}

void zg_core_shutdown(void)
{
    zg_device_shutdown();
    zg_keys_shutdown();
    zg_zll_shutdown();
    zg_zha_shutdown();
    zg_zdp_shutdown();
    zg_ipc_shutdown();
    zg_mt_init();
}

