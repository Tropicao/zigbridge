#include <stdlib.h>
#include <znp.h>
#include "core.h"
#include "zha.h"
#include "zll.h"
#include "sm.h"
#include "aps.h"
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_zdo.h"
#include "mt_util.h"
#include "ipc.h"

/********************************
 * Initialization state machine *
 *******************************/

static ZgSm *_init_sm = NULL;
static uint8_t _initialized = 0;

/* Callback triggered when core initialization is complete */

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
    {
        LOG_INF("Core application is initialized");
        _initialized = 1;
    }
}
static ZgSmState _init_states[] = {
    {mt_sys_reset_dongle, _general_init_cb},
    {mt_sys_nv_write_nwk_key, _general_init_cb},
    {mt_sys_reset_dongle, _general_init_cb},
    {mt_sys_nv_write_coord_flag, _general_init_cb},
    {mt_sys_nv_write_disable_security, _general_init_cb},
    {mt_sys_nv_set_pan_id, _general_init_cb},
    {mt_sys_ping_dongle, _general_init_cb},
    {mt_util_af_subscribe_cmd, _general_init_cb},
    {zg_zll_init, _general_init_cb},
    {zg_zha_init, _general_init_cb},
    {mt_zdo_startup_from_app, _general_init_cb}
};

static int _init_nb_states = sizeof(_init_states)/sizeof(ZgSmState);

/********************************
 *     Commands processing      *
 *******************************/

static void _process_user_command(IpcCommand cmd)
{
    switch(cmd)
    {
        case ZG_IPC_COMMAND_TOUCHLINK:
            if(_initialized)
                zg_zll_start_touchlink();
            else
                LOG_WARN("Core application has not finished initializing, cannot start touchlink");
            break;
        case ZG_IPC_COMMAND_SWITCH_DEMO_LIGHT:
            if(_initialized)
            {
                if(zg_zha_device_is_installed())
                    zg_zha_switch_bulb_state();
                else
                    LOG_WARN("Device is not installed, cannot switch light");
            }
            else
            {
                LOG_WARN("Core application has not finished initializing, cannot switch bulb state");
            }
            break;
        default:
            LOG_WARN("Unsupported command");
            break;
    }
}


/********************************
 *             API              *
 *******************************/

void zg_core_init(void)
{
    LOG_INF("Initializing core application");
    mt_af_register_callbacks();
    mt_sys_register_callbacks();
    mt_zdo_register_callbacks();
    mt_util_register_callbacks();
    zg_aps_init();
    zg_ipc_register_command_cb(_process_user_command);

    _init_sm = zg_sm_create(_init_states, _init_nb_states);
    zg_sm_continue(_init_sm);
}

void zg_core_shutdown(void)
{
    zg_zha_shutdown();
    zg_zll_shutdown();
    zg_aps_shutdown();
}
