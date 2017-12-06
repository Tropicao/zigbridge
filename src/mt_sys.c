#include <stdlib.h>
#include "mt_sys.h"
#include "dbgPrint.h"
#include "mtSys.h"
#include "app.h"
#include "uv.h"
#include "rpc.h"

AppState state;

/********************************
 *     MT SYS callbacks         *
 *******************************/

static uint8_t mt_sys_ping_srsp_cb(PingSrspFormat_t *msg)
{
    AppState *state = NULL;
    log_inf("System ping SRSP received. Capabilities : %04X", msg->Capabilities);
    state = calloc(1, sizeof(AppState));
    if(state)
    {
        *state = APP_STATE_DONGLE_PRESENT;
        state_flag.data = state;
    }
    uv_async_send(&state_flag);

    return 0;
}

static uint8_t mt_sys_reset_ind_cb(ResetIndFormat_t *msg)
{
    log_inf("System reset ind. received. Reason : %s ",
            msg->Reason == 0 ? "power up":(msg->Reason == 1 ? "External":"Watchdog"));
    log_inf("Transport version : %d", msg->TransportRev);
    log_inf("Product ID : %d", msg->ProductId);
    log_inf("Major version : %d", msg->MajorRel);
    log_inf("Minor version : %d", msg->MinorRel);
    log_inf("Hardware version : %d", msg->HwRev);
    state = APP_STATE_DONGLE_UP;
    state_flag.data = (void *)&state;
    uv_async_send(&state_flag);

    return 0;
}

static mtSysCb_t mt_sys_cb = {
    mt_sys_ping_srsp_cb,
    NULL,
    NULL,
    mt_sys_reset_ind_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/********************************
 *          API                 *
 *******************************/

void mt_sys_register_callbacks(void)
{
    sysRegisterCallbacks(mt_sys_cb);
}

void mt_sys_reset_dongle (void)
{
    log_inf("Resetting ZNP");
    ResetReqFormat_t resReq;
    resReq.Type = 1;
    sysResetReq(&resReq);
    rpcWaitMqClientMsg(5000);
}

void mt_sys_ping_dongle(void)
{
    log_inf("Ping dongle");
    sysPing();
}

