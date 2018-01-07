#include <znp.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zha.h"
#include "aps.h"
#include "zcl.h"
#include "mt_af.h"
#include "mt_zdo.h"
#include "mt_sys.h"
#include "sm.h"

/********************************
 *          Constants           *
 *******************************/

#define ZHA_ENDPOINT                            0x2

/********** Format **********/

#define COMMAND_OFF                             0x00
#define COMMAND_ON                              0x01
#define COMMAND_TOGGLE                          0x02
#define COMMAND_OFF_WITH_EFFECT                 0x40
#define COMMAND_ON_WITH_RECALL_GLOBAL_SCENE     0x41

/* Off with effect frame format */
#define LEN_OFF_WITH_EFFECT                     2
#define INDEX_EFFECT_IDENTIFIER                 0
#define INDEX_EFFeCT_VARIANT                    0

/* Data */

/* Scan request */
#define DEVICE_TYPE_COORDINATOR                 0x0
#define DEVICE_TYPE_ROUTER                      0x1
#define DEVICE_TYPE_END_DEVICE                  0x2

#define RX_ON_WHEN_IDLE                         (0x1<<2)
#define RX_OFF_WHEN_IDLE                        (0x0<<2)

/* Identify Request */
#define EXIT_IDENTIFY                           0x0000
#define DEFAULT_IDENTIFY_DURATION               0xFFFF

/********** Application data **********/

/* Off with effect */
#define DELAYED_ALL_OFF                         0x00
#define DYING_LIGHT                             0x01

/* App data */
#define ZHA_PROFIL_ID                           0x0104  /* ZLL */
#define ZHA_DEVICE_ID                           0X0210  /* Extended color light */
#define ZHA_DEVICE_VERSION                      0x2     /* Version 2 */

/********************************
 *          Local variables     *
 *******************************/

static uint16_t _demo_bulb_addr = 0xFFFD;
static uint8_t _demo_bulb_state = 0x1;

static uint16_t _zha_in_clusters[] = {
    ZCL_CLUSTER_ON_OFF};
static uint8_t _zha_in_clusters_num = sizeof(_zha_in_clusters)/sizeof(uint8_t);

static uint16_t _zha_out_clusters[] = {
    ZCL_CLUSTER_ON_OFF};
static uint8_t _zha_out_clusters_num = sizeof(_zha_out_clusters)/sizeof(uint8_t);

/********************************
 * Initialization state machine *
 *******************************/

static ZgSm *_init_sm = NULL;

/* Callback triggered when ZHA initialization is complete */
static InitCompleteCb _init_complete_cb = NULL;

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
    {
        LOG_INF("ZHA application is initialized");
        if(_init_complete_cb)
            _init_complete_cb();
    }
}

static ZgSmState _init_states[] = {
    {zg_zha_register_endpoint, _general_init_cb}
};
static int _init_nb_states = sizeof(_init_states)/sizeof(ZgSmState);
/********************************
 *   ZHA messages callbacks     *
 *******************************/

static void _zha_message_cb(void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    LOG_DBG("Received ZHA data (%d bytes)", len);
    switch(buffer[2])
    {
        default:
            LOG_WARN("Unsupported ZHA command %02X", buffer[2]);
            break;
    }
}

static void _zha_visible_device_cb(uint16_t addr)
{
    LOG_INF("Demo bulb registered with address 0x%04X !", addr);
    _demo_bulb_addr = addr;
}

static void _security_disabled_cb(void)
{
    zg_zha_switch_bulb_state();
}

/********************************
 *          ZLL API             *
 *******************************/

void zg_zha_init(InitCompleteCb cb)
{
    LOG_INF("Initializing ZHA");
    if(cb)
        _init_complete_cb = cb;

    mt_zdo_register_visible_device_cb(_zha_visible_device_cb);
    _init_sm = zg_sm_create(_init_states, _init_nb_states);
    zg_sm_continue(_init_sm);
}

void zg_zha_register_endpoint(SyncActionCb cb)
{
    zg_aps_register_endpoint(   ZHA_ENDPOINT,
                                ZHA_PROFIL_ID,
                                ZHA_DEVICE_ID,
                                ZHA_DEVICE_VERSION,
                                _zha_in_clusters_num,
                                _zha_in_clusters,
                                _zha_out_clusters_num,
                                _zha_out_clusters,
                                _zha_message_cb,
                                cb);
}

void zg_zha_switch_bulb_state(void)
{
    static int sec_enabled = 0;

    if(!sec_enabled)
    {
        LOG_INF("Re-enabling security before sending command");
        mt_sys_nv_write_enable_security(_security_disabled_cb);
        sec_enabled = 1;
        return;
    }

    if(_demo_bulb_addr != 0xFFFD)
    {
        _demo_bulb_state = !_demo_bulb_state;
        LOG_INF("Switch : Bulb 0x%4X - State %s", _demo_bulb_addr, _demo_bulb_state ? "ON":"OFF");
        zg_zha_set_bulb_state(_demo_bulb_addr, _demo_bulb_state);
    }
}

void zg_zha_set_bulb_state(uint16_t addr, uint8_t state)
{
    uint8_t command = state ? COMMAND_ON:COMMAND_OFF;

    zg_aps_send_data(addr,
            0xABCD,
            ZHA_ENDPOINT,
            0x0B,
            ZCL_CLUSTER_ON_OFF,
            command,
            NULL,
            0,
            NULL);
}



