#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <znp.h>
#include "mt_af.h"

static AfIncomingMessageCb _af_incoming_msg_cb = NULL;
static SyncActionCb sync_action_cb = NULL;
static uint8_t _transaction_id = 0;

/********************************
 *          Data Types          *
 *******************************/

typedef enum
{
    SHORT_ADDR_MODE = 2,
    EXT_ADDR_MODE = 3,
}DstAddrMode;


/********************************
 *       Constant data          *
 *******************************/

#define DATA_REQUEST_DEFAULT_OPTIONS        0x00
#define DATA_REQUEST_DEFAULT_RADIUS         0x5
#define DEFAULT_LATENCY                     0

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
    {
        LOG_WARN("AF data request status : %02X", msg->Status);
    }
    else
    {
        LOG_INF("AF data request status : %02X", msg->Status);
        _transaction_id++;
    }
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _data_request_ext_srsp_cb(DataRequestExtSrspFormat_t *msg)
{
    if(msg->Status != 0)
    {
        LOG_WARN("AF data request ext status : %02X", msg->Status);
    }
    else
    {
        LOG_INF("AF data request ext status : %02X", msg->Status);
        _transaction_id++;
    }
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

    /* TODO : add parsing for huge buffer, ie with multiple AF_DATA_RETRIEVE
    */
    if(_af_incoming_msg_cb)
        _af_incoming_msg_cb(msg->DstEndpoint, msg->Data, msg->Len);

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

void mt_af_register_endpoint(   uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                SyncActionCb cb)
{
    RegisterFormat_t req;
    uint8_t index;

    LOG_INF("Registering new endpoint with profile 0x%4X", profile);
    if(cb)
        sync_action_cb = cb;
    req.EndPoint = endpoint;
    req.AppProfId = profile;
    req.AppDeviceId = device_id;
    req.AppDevVer = device_ver;
    req.LatencyReq = DEFAULT_LATENCY;
    req.AppNumInClusters = in_clusters_num;
    for(index = 0; index < in_clusters_num; index++)
        req.AppInClusterList[index] = in_clusters_list[index];
    req.AppNumOutClusters = out_clusters_num;
    for(index = 0; index < out_clusters_num; index++)
        req.AppOutClusterList[index] = out_clusters_list[index];
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

void mt_af_register_incoming_message_callback(AfIncomingMessageCb cb)
{
    _af_incoming_msg_cb = cb;
}

void mt_af_send_data_request_ext(   uint16_t dst_addr,
                                    uint16_t dst_pan,
                                    uint8_t src_endpoint,
                                    uint8_t dst_endpoint,
                                    uint16_t cluster,
                                    uint16_t len,
                                    void *data,
                                    SyncActionCb cb)
{

    if(len == 0 || !data)
    {
        LOG_ERR("Cannot send AF_DATA_REQUEST_EXT (%s)", data ? "length is invalid":"no data provided");
        return;
    }

    LOG_DBG("Sending AF_DATA_REQUEST_EXT");
    if(cb)
        sync_action_cb = cb;
    DataRequestExtFormat_t req;
    req.DstAddrMode = SHORT_ADDR_MODE;
    memset(req.DstAddr, 0, sizeof(req.DstAddr));
    req.DstAddr[0] = dst_addr & 0xFF;
    req.DstAddr[1] = (dst_addr >> 8) & 0xFF;
    req.DstEndpoint = dst_endpoint;
    req.DstPanID = dst_pan;
    req.SrcEndpoint = src_endpoint;
    req.ClusterId = cluster;
    req.TransId = _transaction_id;
    req.Options = DATA_REQUEST_DEFAULT_OPTIONS;
    req.Radius = DATA_REQUEST_DEFAULT_RADIUS;
    req.Len = len;
    memcpy(req.Data, data, len);
    afDataRequestExt(&req);
}
