
#include <stdlib.h>
#include "mt_zdo.h"
#include "dbgPrint.h"
#include "mtZdo.h"
#include "app.h"
#include "uv.h"
#include "rpc.h"

AppState state;

/********************************
 *       Constant data          *
 *******************************/
#define SCAN_ALL_CHANNELS_VALUE     0x07FFF800

/********************************
 *     MT ZDO callbacks         *
 *******************************/

static uint8_t mt_zdo_nwk_discovery_srsp_cb(NwkDiscoveryCnfFormat_t *msg)
{
    log_inf("ZDO Network discovery status : %02d", msg->Status);
    state = APP_STATE_ZDO_DISCOVERY_SENT;
    state_flag.data = (void *)&state;
    uv_async_send(&state_flag);

    return 0;
}

static mtZdoCb_t mt_zdo_cb = {
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
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_zdo_nwk_discovery_srsp_cb,
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

void mt_zdo_register_callbacks(void)
{
    zdoRegisterCallbacks(mt_zdo_cb);
}

void mt_zdo_nwk_discovery_req(void)
{
    NwkDiscoveryReqFormat_t req;
    int index = 0;
    log_inf("Sending ZDO network discover request");
    for(index = 0; index<4; index++)
        req.ScanChannels[index] = (SCAN_ALL_CHANNELS_VALUE & (0xFF<<(index*8))) >> (index*8);
    req.ScanDuration = 5;
    zdoNwkDiscoveryReq(&req);
}

