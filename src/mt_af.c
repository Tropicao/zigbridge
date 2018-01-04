#include <stdlib.h>
#include <string.h>
#include <znp.h>
#include "mt_af.h"
#include "uv.h"

static ZgZllCb _zll_data_cb;
static SyncActionCb sync_action_cb = NULL;

/********************************
 *       Constant data          *
 *******************************/

#define ZLL_ENDPOINT            0x1     /* Endpoint 1 */
#define ZLL_PROFIL_ID           0xC05E  /* ZLL */
#define ZLL_DEVICE_ID           0X0210  /* Extended color light */
#define ZLL_DEVICE_VERSION      0x2     /* Version 2 */
#define ZLL_LATENCY             0x00    /* No latency */
#define ZLL_NUM_IN_CLUSTERS     0x1     /* Only one input cluster defined */
#define ZLL_IN_CLUSTERS_ID      0x1000  /* Commissionning cluster */
#define ZLL_NUM_OUT_CLUSTERS    0x1     /* Only one output cluster */
#define ZLL_OUT_CLUSTERS_ID     0x1000  /* Commissioning cluster */

#define ZHA_ENDPOINT            0x2     /* Endpoint 2 */
#define ZHA_PROFIL_ID           0x0104  /* ZHA */
#define ZHA_DEVICE_ID           0X0050  /* Home gateway */
#define ZHA_DEVICE_VERSION      0x2     /* Version 2 */
#define ZHA_LATENCY             0x00    /* No latency */
#define ZHA_NUM_IN_CLUSTERS     0x1     /* Only one input cluster defined */
#define ZHA_IN_CLUSTERS_ID      0x0006  /* On/Off cluster */
#define ZHA_NUM_OUT_CLUSTERS    0x1     /* Only one output cluster */
#define ZHA_OUT_CLUSTERS_ID     0x0006  /* On/Off cluster */

#define ZLL_DEVICE_ADDR         0xFFFF
#define ZLL_DST_ENDPOINT        0x01
#define ZLL_SRC_ENDPOINT        0x01
#define ZLL_CLUSTER_ID          0x1000
#define ZLL_TRANS_ID            180
#define ZLL_OPTIONS             0x00
#define ZLL_RADIUS              0x5
#define ZLL_LEN                 9

#define ZHA_DST_ENDPOINT        0x0B
#define ZHA_SRC_ENDPOINT        0x02
#define ZHA_CLUSTER_ID          0x0006
#define ZHA_TRANS_ID            180
#define ZHA_OPTIONS             0x00
#define ZHA_RADIUS              0x5

static uint8_t zll_scan_data[] = {  0x11,   // Frame control
                                    0x2C,   // Transaction Sequence Number
                                    0x00,   // Command ID
                                    0x4C,   // Payload start : Interpan Transaction ID, 4 bytes
                                    0xAD,
                                    0xB9,
                                    0xE2,
                                    0x05,   // 0x1+0x4 : Router + Rx On when Idle
                                    0x12 }; // Touchlink data - payload end

static uint8_t zll_identify_data[] = {  0x11,   // Frame control
                                        0x2C,   // Transaction Sequence Number
                                        0x06,   // Command ID
                                        0x4C,   // Payload start : Interpan Transaction ID, 4 bytes
                                        0xAD,
                                        0xB9,
                                        0xE2,
                                        0x03,   // 0xffff : identify for a time known by device
                                        0x00 }; // Payload end

static uint8_t zll_factory_reset_data[] = {  0x11,   // Frame control
                                        0x2C,   // Transaction Sequence Number
                                        0x07,   // Command ID
                                        0x4C,   // Payload start : Interpan Transaction ID, 4 bytes
                                        0xAD,
                                        0xB9,
                                        0xE2};

static uint8_t zha_on_off_with_effect_data[] = { 0x11,  // Frame control
                                                0x59,   // Transac number
                                                0x40,   // Command
                                                0x00,   // Command : Off
                                                0x00 };
static uint8_t zha_on_off_with_effect_data_size = sizeof(zha_on_off_with_effect_data);



#define ZLL_INTERPAN_CMD                0x1
static uint8_t zll_inter_pan_data[] =   {0xB};

#define ZLL_INTERPAN_CMD_2              0x2
static uint8_t zll_inter_pan_data_2[] = {0x1};

/********************************
 *      MT AF callbacks         *
 *******************************/

static uint8_t _register_srsp_cb(RegisterSrspFormat_t *msg)
{
    LOG_INF("AF register status status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _data_request_srsp_cb(DataRequestSrspFormat_t *msg)
{
    if(msg->Status != 0)
        LOG_WARN("AF request status status : %02X", msg->Status);
    else
        LOG_INF("AF request status status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _data_request_ext_srsp_cb(DataRequestExtSrspFormat_t *msg)
{
    if(msg->Status != 0)
        LOG_WARN("AF request status status : %02X", msg->Status);
    else
        LOG_INF("AF request status status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _inter_pan_ctl_srsp_cb(InterPanCtlSrspFormat_t *msg)
{
    if(msg->Status != 0)
        LOG_WARN("AF interpan ctl status status : %02X", msg->Status);
    else
        LOG_INF("AF interpan ctl status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _incoming_msg_ext_cb(IncomingMsgExtFormat_t *msg)
{
    uint8_t *buffer = NULL;
    int index = 0;
    LOG_INF("Extended AF message received");
    LOG_INF("Group id : 0x%04X", msg->GroupId);
    LOG_INF("Cluster id : 0x%04X", msg->ClusterId);
    LOG_INF("Source addr mode : 0x%02X", msg->SrcAddrMode);
    LOG_INF("Source addr : 0x%016X", msg->SrcAddr);
    LOG_INF("Source Endpoint : 0x%02X", msg->SrcEndpoint);
    LOG_INF("Source PAN id : 0x%04X", msg->SrcPanId);
    LOG_INF("Dest Endpoint : 0x%02X", msg->DstEndpoint);
    LOG_INF("Was Broadcast : 0x%02X", msg->WasBroadcast);
    LOG_INF("Link Quality : 0x%02X", msg->LinkQuality);
    LOG_INF("Security Use : 0x%02X", msg->SecurityUse);
    LOG_INF("Timestamp : 0x%08X", msg->TimeStamp);
    LOG_INF("Transaction sequence num : 0x%02X", msg->TransSeqNum);
    LOG_INF("Length : %d", msg->Len);
    if(     msg->DstEndpoint == ZLL_ENDPOINT &&
            msg->ClusterId == ZLL_CLUSTER_ID &&
            _zll_data_cb)
    {
        LOG_INF("Received message is a ZLL commissioning message");
        buffer = calloc(msg->Len, sizeof(uint8_t));
        /* TODO : add parsing for huge buffer, ie with multiple AF_DATA_RETRIEVE
         */
        for (index = 0; index<msg->Len; index ++)
            buffer[index] = msg->Data[index];

        _zll_data_cb(buffer, msg->Len);
        free(buffer);
    }

    return 0;
}

static uint8_t _data_confirm_cb(DataConfirmFormat_t *msg)
{
    if(msg->Status != 0)
        LOG_WARN("AF data request status : %02X", msg->Status);
    else
        LOG_INF("AF data request status : %02X", msg->Status);

    return 0;
}



static mtAfCb_t mt_af_cb = {
    _register_srsp_cb,
    _data_request_srsp_cb,
    _data_request_ext_srsp_cb,
    _data_confirm_cb,
    NULL,
    _incoming_msg_ext_cb,
    NULL,
    NULL,
    _inter_pan_ctl_srsp_cb,
};

/********************************
 *          API                 *
 *******************************/

void mt_af_register_callbacks(void)
{
    afRegisterCallbacks(mt_af_cb);
}

void mt_af_register_zll_endpoint(SyncActionCb cb)
{
    RegisterFormat_t req;

    LOG_INF("Registering ZLL endpoint");
    if(cb)
        sync_action_cb = cb;
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

void mt_af_register_zha_endpoint(SyncActionCb cb)
{
    RegisterFormat_t req;

    LOG_INF("Registering ZHA endpoint");
    if(cb)
        sync_action_cb = cb;
    req.EndPoint = ZHA_ENDPOINT;
    req.AppProfId = ZHA_PROFIL_ID;
    req.AppDeviceId = ZHA_DEVICE_ID;
    req.AppDevVer = ZHA_DEVICE_VERSION;
    req.LatencyReq = ZHA_LATENCY;
    req.AppNumInClusters = ZHA_NUM_IN_CLUSTERS;
    req.AppInClusterList[0] = ZHA_IN_CLUSTERS_ID;
    req.AppNumOutClusters = ZHA_NUM_OUT_CLUSTERS;
    req.AppOutClusterList[0] = ZHA_OUT_CLUSTERS_ID;
    afRegister(&req);
}

void mt_af_set_inter_pan_endpoint(SyncActionCb cb)
{
    InterPanCtlFormat_t req;

    LOG_INF("Setting inter-pan endpoint");
    if(cb)
        sync_action_cb = cb;
    req.Command = ZLL_INTERPAN_CMD_2;
    memcpy(req.Data, zll_inter_pan_data_2, sizeof(zll_inter_pan_data_2)/sizeof(uint8_t));
    afInterPanCtl(&req);
}

void mt_af_set_inter_pan_channel(SyncActionCb cb)
{
    InterPanCtlFormat_t req;

    LOG_INF("Setting inter-pan channel");
    if(cb)
        sync_action_cb = cb;
    req.Command = ZLL_INTERPAN_CMD;
    memcpy(req.Data, zll_inter_pan_data, sizeof(zll_inter_pan_data)/sizeof(uint8_t));
    afInterPanCtl(&req);
}

void mt_af_send_zll_scan_request(SyncActionCb cb)
{
    LOG_INF("Sending zll scan request");
    if(cb)
        sync_action_cb = cb;
    DataRequestExtFormat_t req;
    req.DstAddrMode =0x2;
    memset(req.DstAddr, 0, 8);
    req.DstAddr[0] = 0xFF;
    req.DstAddr[1] = 0xFF;
    req.DstEndpoint = 0xFE;
    req.DstPanID = 0xFFFF;
    req.SrcEndpoint = ZLL_SRC_ENDPOINT;
    req.ClusterId = ZLL_CLUSTER_ID;
    req.TransId = ZLL_TRANS_ID;
    req.Options = ZLL_OPTIONS;
    req.Radius = ZLL_RADIUS;
    req.Len = ZLL_LEN;
    memcpy(req.Data, zll_scan_data, ZLL_LEN);
    afDataRequestExt(&req);
}

void mt_af_send_zll_identify_request(SyncActionCb cb)
{
    LOG_INF("Sending zll identify request");
    if(cb)
        sync_action_cb = cb;
    DataRequestExtFormat_t req;
    req.DstAddrMode =0x2;
    memset(req.DstAddr, 0, 8);
    req.DstAddr[0] = 0xFF;
    req.DstAddr[1] = 0xFF;
    req.DstEndpoint = 0xFE;
    req.DstPanID = 0xFFFF;
    req.SrcEndpoint = ZLL_SRC_ENDPOINT;
    req.ClusterId = ZLL_CLUSTER_ID;
    req.TransId = ZLL_TRANS_ID;
    req.Options = ZLL_OPTIONS;
    req.Radius = ZLL_RADIUS;
    req.Len = ZLL_LEN;
    memcpy(req.Data, zll_identify_data, ZLL_LEN);
    afDataRequestExt(&req);
}

void mt_af_send_zll_factory_reset_request(SyncActionCb cb)
{
    LOG_INF("Sending zll factory reset request");
    if(cb)
        sync_action_cb = cb;
    DataRequestExtFormat_t req;
    req.DstAddrMode =0x2;
    memset(req.DstAddr, 0, 8);
    req.DstAddr[0] = 0xFF;
    req.DstAddr[1] = 0xFF;
    req.DstEndpoint = 0xFE;
    req.DstPanID = 0xFFFF;
    req.SrcEndpoint = ZLL_SRC_ENDPOINT;
    req.ClusterId = ZLL_CLUSTER_ID;
    req.TransId = ZLL_TRANS_ID;
    req.Options = ZLL_OPTIONS;
    req.Radius = ZLL_RADIUS;
    req.Len = ZLL_LEN-2;
    memcpy(req.Data, zll_factory_reset_data, ZLL_LEN-2);
    afDataRequestExt(&req);
}

void mt_af_register_zll_callback(ZgZllCb cb)
{
    _zll_data_cb = cb;
}

void mt_af_switch_bulb_state(uint16_t addr, uint8_t state)
{
    int len = zha_on_off_with_effect_data_size;
    LOG_INF("Sending zha %s request to 0x%04X", state ? "ON":"OFF", addr);
    DataRequestExtFormat_t req;
    req.DstAddrMode =0x2;
    memset(req.DstAddr, 0, 8);
    req.DstAddr[0] = addr & 0xFF;
    req.DstAddr[1] = (addr >> 8) & 0xFF;
    LOG_INF("Dst addr : 0x%02X%02X", req.DstAddr[1], req.DstAddr[0]);
    req.DstEndpoint = 0x0B;
    req.DstPanID = 0xABCD;
    req.SrcEndpoint = ZHA_SRC_ENDPOINT;
    req.ClusterId = ZHA_CLUSTER_ID;
    req.TransId = ZHA_TRANS_ID;
    req.Options = ZHA_OPTIONS;
    req.Radius = ZHA_RADIUS;
    if(state)
    {
        zha_on_off_with_effect_data[2] = 0x1;
        len = zha_on_off_with_effect_data_size-2;
    }
    else
    {
        zha_on_off_with_effect_data[2] = 0x40;
        len = zha_on_off_with_effect_data_size;
    }
    req.Len = len;

    memcpy(req.Data, zha_on_off_with_effect_data, len);
    afDataRequestExt(&req);
}
