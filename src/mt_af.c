#include <stdlib.h>
#include "mt_af.h"
#include "dbgPrint.h"
#include "mtAf.h"
#include "app.h"
#include "uv.h"
#include "rpc.h"
#include <string.h>

AppState state;

/********************************
 *       Constant data          *
 *******************************/

#define ZLL_ENDPOINT            0x1     /* Endpoint 1 */
#define ZLL_PROFIL_ID           0xC05E  /* ZLL */
#define ZLL_DEVICE_ID           0X0210  /*Extended color light */
#define ZLL_DEVICE_VERSION      0x2     /* Version 2 */
#define ZLL_LATENCY             0x00    /* No latency */
#define ZLL_NUM_IN_CLUSTERS     0x1     /* Only one input cluster defined */
#define ZLL_IN_CLUSTERS_ID      0x1000  /* Commissionning cluster */
#define ZLL_NUM_OUT_CLUSTERS    0x1     /* Only one output cluster */
#define ZLL_OUT_CLUSTERS_ID     0x1000  /* Commissioning cluster */

/********************************
 *      MT AF callbacks         *
 *******************************/

static uint8_t mt_af_register_srsp_cb(RegisterSrspFormat_t *msg)
{
    LOG_INF("AF register status status : %02X", msg->Status);
    state = APP_STATE_ZLL_REGISTERED;
    state_flag.data = (void *)&state;
    uv_async_send(&state_flag);

    return 0;
}


static mtAfCb_t mt_af_cb = {
    mt_af_register_srsp_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/********************************
 *          API                 *
 *******************************/

void mt_af_register_callbacks(void)
{
    afRegisterCallbacks(mt_af_cb);
}

void mt_af_register_zll_endpoint()
{
    RegisterFormat_t req;

    LOG_INF("Registering ZLL endpoint");
    req.EndPoint = ZLL_ENDPOINT;
    req.AppProfId = ZLL_PROFIL_ID;
    req.AppDeviceId = ZLL_DEVICE_ID;
    req.AppDevVer = ZLL_DEVICE_VERSION;
    req.LatencyReq = ZLL_LATENCY;
    req.AppNumInClusters = ZLL_NUM_IN_CLUSTERS;
    req.AppInClusterList[0] = ZLL_IN_CLUSTERS_ID;
    req.AppNumOutClusters = ZLL_NUM_OUT_CLUSTERS;
    req.AppOutClusterList[0] = ZLL_OUT_CLUSTERS_ID;
    afRegister(&req);
}

void mt_af_send_zll_scan_request()
{
    DataRequestFormat_t req;

    LOG_INF("Sending ZLL scan request");
    afDataRequest(&req);
}

