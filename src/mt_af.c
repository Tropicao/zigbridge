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

#define ZLL_DEVICE_ADDR         0xFFFF
#define ZLL_DST_ENDPOINT        0x01
#define ZLL_SRC_ENDPOINT        0x01
#define ZLL_CLUSTER_ID          0x1000
#define ZLL_TRANS_ID            0x67
#define ZLL_OPTIONS             0x00
#define ZLL_RADIUS              0x5
#define ZLL_LEN                 9

#define ZLL_SET_INTERPAN        0x1
static uint8_t zll_scan_data[] ={   0x11,   // Frame control
                                    0x2C,   // Transaction Sequence Number
                                    0x00,   // Command ID
                                    0x4C,   // Payload start : Interpan Transaction ID, 4 bytes
                                    0xAD,
                                    0xB9,
                                    0xE2,
                                    0x05,   // 0x1+0x4 : Router + Rx On when Idle
                                    0x12 }; // Touchlink data

static uint8_t zll_interpan_endpoint[] = {0xB};


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

static uint8_t mt_af_data_request_srsp_cb(DataRequestSrspFormat_t *msg)
{
    LOG_INF("AF request status status : %02X", msg->Status);
    state = APP_STATE_ZLL_SCAN_REQUEST_SENT;
    state_flag.data = (void *)&state;
    uv_async_send(&state_flag);

    return 0;
}

static uint8_t mt_af_inter_pan_ctl_srsp_cb(InterPanCtlSrspFormat_t *msg)
{
    if(msg->Status != 0)
        LOG_WARN("AF request status status : %02X", msg->Status);
    else
        LOG_INF("AF request status status : %02X", msg->Status);
    state = APP_STATE_INTER_PAN_CTL_SENT;
    state_flag.data = (void *)&state;
    uv_async_send(&state_flag);

    return 0;
}


static mtAfCb_t mt_af_cb = {
    mt_af_register_srsp_cb,
    mt_af_data_request_srsp_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_af_inter_pan_ctl_srsp_cb,
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

void mt_af_set_inter_pan_endpoint(void)
{
    InterPanCtlFormat_t req;

    LOG_INF("Setting inter-pan messages mode");
    req.Command = ZLL_SET_INTERPAN;
    memcpy(req.Data, zll_interpan_endpoint, 0x1);
    afInterPanCtl(&req);
}

void mt_af_send_zll_scan_request()
{
    DataRequestFormat_t req;

    LOG_INF("sending zll scan request");
    req.DstAddr = ZLL_DEVICE_ADDR;
    req.DstEndpoint = ZLL_DST_ENDPOINT;
    req.SrcEndpoint = ZLL_SRC_ENDPOINT;
    req.ClusterID = ZLL_CLUSTER_ID;
    req.TransID = ZLL_TRANS_ID;
    req.Options = ZLL_OPTIONS;
    req.Radius = ZLL_RADIUS;
    req.Len = ZLL_LEN;
    memcpy(req.Data, zll_scan_data, ZLL_LEN);
    afDataRequest(&req);
}

