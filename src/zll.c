#include "zll.h"
#include "mt_af.h"
#include "sm.h"
#include <dbgPrint.h>
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_zdo.h"
#include "mt_util.h"
#include <stdlib.h>
#include <uv.h>

/********************************
 *          Constants           *
 *******************************/

#define INDEX_FCS               0x0
#define INDEX_TRANS_SEQ_NUM     0x1
#define INDEX_COMMAND           0x2

#define COMMAND_SCAN_REQUEST                    0x00
#define COMMAND_SCAN_RESPONSE                   0x01
#define COMMAND_DEVICE_INFORMATION_REQUEST      0x02
#define COMMAND_IDENTIFY_REQUEST                0x06
#define COMMAND_FACTORY_NEW_REQUEST             0x07
#define COMMAND_NETWORK_START_REQUEST           0x10
#define COMMAND_NETWORK_JOIN_ROUTER_REQUEST     0x12
#define COMMAND_NETWORK_JOIN_END_DEVICE_REQUEST 0x14
#define COMMAND_NETWORK_UPDATE_REQUEST          0x16

#define SCAN_TIMEOUT                            0.25


/********************************
 *          Local variables     *
 *******************************/
static uv_timer_t _scan_timeout_timer;
static uv_timer_t _identify_timer;
static uint8_t _stop_scan = 0;
static uint16_t _demo_bulb_addr = 0xFFFD;
static uint8_t _demo_bulb_state = 0x1;

/********************************
 * Initialization state machine *
 *******************************/

static ZgSm *_init_sm = NULL;

/* Callback triggered when ZLL initialization is complete */
InitCompleteCb _init_complete_cb = NULL;

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
    {
        LOG_INF("ZLL Gateway is initialized");
        if(_init_complete_cb)
            _init_complete_cb();
    }
}

static ZgSmState _init_states[] = {
    {mt_sys_nv_write_clear_flag, _general_init_cb},
    {mt_sys_reset_dongle, _general_init_cb},
    {mt_sys_nv_write_nwk_key, _general_init_cb},
    {mt_sys_reset_dongle, _general_init_cb},
    {mt_sys_nv_write_coord_flag, _general_init_cb},
    {mt_sys_nv_write_disable_security, _general_init_cb},
    {mt_sys_nv_set_pan_id, _general_init_cb},
    {mt_sys_ping_dongle, _general_init_cb},
    {mt_af_register_zll_endpoint, _general_init_cb},
    {mt_af_register_zha_endpoint, _general_init_cb},
    {mt_af_set_inter_pan_endpoint, _general_init_cb},
    {mt_af_set_inter_pan_channel, _general_init_cb},
    {mt_util_af_subscribe_cmd, _general_init_cb},
    {mt_zdo_startup_from_app, _general_init_cb}
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
    mt_af_send_zll_scan_request(_scan_request_sent);
}

static void _scan_timeout_cb(uv_timer_t *s __attribute__((unused)))
{
    static int scan_counter = 0;

    scan_counter++;
    if(scan_counter < 5 && !_stop_scan)
        _send_single_scan_request();
    else
        uv_unref((uv_handle_t *) &_scan_timeout_timer);
}

static void _send_five_scan_requests()
{
    _stop_scan = 0;
    uv_timer_init(uv_default_loop(), &_scan_timeout_timer);
    _send_single_scan_request();
}

/********************************
 *   ZLL messages callbacks     *
 *******************************/

static void _identify_delay_timeout_cb(uv_timer_t *t)
{
    uv_unref((uv_handle_t *)t);
    mt_af_send_zll_factory_reset_request(NULL);
}

static uint8_t _processScanResponse(void *data __attribute__((unused)), int len __attribute__((unused)))
{
    LOG_INF("A device is ready to install");
    _stop_scan = 1;
    mt_af_send_zll_identify_request(NULL);
    uv_timer_init(uv_default_loop(), &_identify_timer);
    uv_timer_start(&_identify_timer, _identify_delay_timeout_cb, 3000, 0);
    return 0;
}

static void _zll_message_cb(void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len < INDEX_COMMAND+1)
        return;

    LOG_DBG("Received ZLL data (%d bytes)", len);
    switch(buffer[INDEX_COMMAND])
    {
        case COMMAND_SCAN_RESPONSE:
            LOG_INF("Received scan response");
            _processScanResponse(buffer, len);
            break;
        default:
            LOG_WARN("Unsupported ZLL commissionning commande %02X", buffer[INDEX_COMMAND]);
            break;
    }
}

void _zll_visible_device_cb(uint16_t addr)
{
    LOG_INF("Demo bulb registered with address 0x%04X !", addr);
    _demo_bulb_addr = addr;
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
    mt_af_register_callbacks();
    mt_sys_register_callbacks();
    mt_zdo_register_callbacks();
    mt_util_register_callbacks();
    mt_af_register_zll_callback(_zll_message_cb);
    mt_zdo_register_visible_device_cb(_zll_visible_device_cb);
    zg_sm_continue(_init_sm);
}

void zg_zll_send_scan_request(void)
{

}

void zg_zll_register_scan_response_callback(void)
{

}

void zg_zll_send_identify_request(void)
{

}

void zg_zll_send_factory_reset_request(void)
{

}

void zg_zll_send_join_router_request(void)
{

}

void zg_zll_start_touchlink(void)
{
    zg_sm_destroy(_init_sm);
    LOG_INF("Starting touchlink procedure");
    _send_five_scan_requests();
}

void zg_zll_switch_bulb_state(void)
{
    static int sec_enabled = 0;

    if(!sec_enabled)
    {
        mt_sys_nv_write_enable_security(NULL);
        sec_enabled = 1;
    }

    if(_demo_bulb_addr != 0xFFFD)
    {
        LOG_INF("Switch : Bulb 0x%4X - State %s", _demo_bulb_addr, _demo_bulb_state ? "ON":"OFF");
        _demo_bulb_state = !_demo_bulb_state;
        mt_af_switch_bulb_state(_demo_bulb_addr, _demo_bulb_state);
    }
}



