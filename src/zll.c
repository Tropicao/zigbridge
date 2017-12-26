#include "zll.h"
#include "mt_af.h"
#include "sm.h"
#include <dbgPrint.h>
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_zdo.h"
#include <stdlib.h>

/********************************
 * Initialization state machine *
 *******************************/
static ZgSm *_init_sm = NULL;

static void _general_init_cb(void)
{
    if(zg_sm_continue(_init_sm) != 0)
        LOG_INF("ZLL Gateway is initialized");
}

static ZgSmState _init_states[] = {
    {mt_sys_nv_write_clear_flag, _general_init_cb},
    {mt_sys_reset_dongle, _general_init_cb},
    {mt_sys_nv_write_coord_flag, _general_init_cb},
    {mt_sys_nv_write_disable_security, _general_init_cb},
    {mt_sys_nv_set_pan_id, _general_init_cb},
    {mt_sys_ping_dongle, _general_init_cb},
    {mt_af_register_zll_endpoint, _general_init_cb},
    {mt_af_set_inter_pan_endpoint, _general_init_cb},
    {mt_zdo_startup_from_app, _general_init_cb}
};
static int _init_nb_states = sizeof(_init_states)/sizeof(ZgSmState);

/********************************
 *   ZLL messages callbacks     *
 *******************************/

static void _zll_message_cb(void *data, int len)
{
    if(!data || len == 0)
        return;

    LOG_DBG("Received ZLL data (%d bytes)", len);
}

/********************************
 *          ZLL API             *
 *******************************/

void zg_zll_init()
{
    LOG_INF("Initializing ZLL");
    _init_sm = zg_sm_create(_init_states, _init_nb_states);
    mt_af_register_callbacks();
    mt_sys_register_callbacks();
    mt_zdo_register_callbacks();
    mt_af_register_zll_callback(_zll_message_cb);
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

