#include <stdlib.h>
#include "mt_sys.h"
#include "dbgPrint.h"
#include "mtSys.h"
#include "app.h"
#include "uv.h"
#include "rpc.h"

AppState state;

/********************************
 *       Constant data          *
 *******************************/

#define MT_SYS_RESET_TIMEOUT_MS     5000
#define MT_SYS_PING_TIMEOUT_MS      50

#define MT_CAP_SYS      0x0001
#define MT_CAP_MAC      0x0002
#define MT_CAP_NWK      0x0004
#define MT_CAP_AF       0x0008
#define MT_CAP_ZDO      0x0010
#define MT_CAP_SAPI     0x0020
#define MT_CAP_UTIL     0x0040
#define MT_CAP_DEBUG    0x0080
#define MT_CAP_APP      0x0100
#define MT_CAP_ZOAD     0x0200


/********************************
 *     MT SYS callbacks         *
 *******************************/

static uint8_t mt_sys_ping_srsp_cb(PingSrspFormat_t *msg)
{
    AppState *state = NULL;
    LOG_INF("System ping SRSP received. Capabilities : %04X", msg->Capabilities);
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
    LOG_INF("System reset ind. received. Reason : %s ",
            msg->Reason == 0 ? "power up":(msg->Reason == 1 ? "External":"Watchdog"));
    LOG_INF("Transport version : %d", msg->TransportRev);
    LOG_INF("Product ID : %d", msg->ProductId);
    LOG_INF("Major version : %d", msg->MajorRel);
    LOG_INF("Minor version : %d", msg->MinorRel);
    LOG_INF("Hardware version : %d", msg->HwRev);
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
    LOG_INF("Resetting ZNP");
    ResetReqFormat_t resReq;
    resReq.Type = 1;
    sysResetReq(&resReq);
    rpcWaitMqClientMsg(MT_SYS_RESET_TIMEOUT_MS);
}

void mt_sys_ping_dongle(void)
{
    LOG_INF("Ping dongle");
    sysPing();
    rpcWaitMqClientMsg(MT_SYS_PING_TIMEOUT_MS);
}

