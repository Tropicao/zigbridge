#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <znp.h>
#include "mt_zdo.h"

/********************************
 *       Constant data          *
 *******************************/
#define SCAN_ALL_CHANNELS_VALUE             0x07FFF800
#define MT_ZDO_NWK_DISCOVERY_TIMEOUT_MS     50

#define DEVICE_ANNCE_CAPABILITIES           0x8E

#define ROUTE_REQ_OPTIONS                   0x00
#define ROUTE_REQ_RADIUS                    0x05

static const uint32_t scan_param = SCAN_ALL_CHANNELS_VALUE;
static SyncActionCb sync_action_cb = NULL;
static void (*_zdo_tc_dev_ind_cb)(uint16_t addr, uint64_t ext_addr) = NULL;
static void (*_zdo_active_ep_rsp_cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list) = NULL;

/********************************
 *     MT ZDO callbacks         *
 *******************************/

static uint8_t mt_zdo_active_ep_rsp_cb(ActiveEpRspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
    {
        LOG_ERR("Error on ACTIVE ENDPOINT SRSP : %s", znp_strerror(msg->Status));
        return 1;
    }
    LOG_INF("MT_ZDO_ACTIVE_EP_RSP received");
    if(_zdo_active_ep_rsp_cb)
        _zdo_active_ep_rsp_cb(msg->NwkAddr, msg->ActiveEPCount, msg->ActiveEPList);

    return 0;
}


static uint8_t mt_zdo_state_change_ind_cb(uint8_t zdoState)
{
    LOG_INF("New ZDO state : 0x%02X", zdoState);
    if(zdoState == 0x09 && sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t mt_zdo_nwk_discovery_srsp_cb(NwkDiscoveryCnfFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_ERR("Error sending ZDO discovery request : %s", znp_strerror(msg->Status));
    else
        LOG_INF("ZDO disocvery request sent");

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t mt_zdo_beacon_notify_ind_cb(BeaconNotifyIndFormat_t *msg)
{
    uint8_t index = 0;
    if(!msg)
        LOG_WARN("Beacon notification received without data");
    for(index = 0; index < msg->BeaconCount; index ++)
    {
        LOG_INF("=== New visible device in range ===");
        LOG_INF("Addr : 0x%04X - PAN : 0x%04X - Channel : 0x%02X",
                msg->BeaconList[index].SrcAddr, msg->BeaconList[index].PanId, msg->BeaconList[index].LogicalChannel);
        LOG_INF("===================================");
    }
    return 0;
}

static uint8_t mt_zdo_tc_dev_ind_cb(TcDevIndFormat_t *msg)
{
    if(!msg)
        LOG_WARN("Device indication received without data");
    LOG_INF("============== Device indication ============");
    LOG_INF("Addr : 0x%04X - ExtAddr : 0x%016X - Parent : 0x%04X",
            msg->SrcNwkAddr, msg->ExtAddr, msg->ParentNwkAddr);
    LOG_INF("=============================================");
    if(_zdo_tc_dev_ind_cb)
        _zdo_tc_dev_ind_cb(msg->SrcNwkAddr, msg->ExtAddr);
    return 0;
}

static uint8_t mt_zdo_device_annce_srsp_cb(DeviceAnnceSrspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_WARN("Error sending device announce request : %s", znp_strerror(msg->Status));
    else
        LOG_INF("Device announce request sent");

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t mt_zdo_ext_route_disc_srsp_cb(ExtRouteDiscSrspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_WARN("Error sending route request : %s", znp_strerror(msg->Status));
    else
        LOG_INF("Route request sent");

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t mt_zdo_active_ep_req_srsp_cb(ActiveEpReqSrspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_WARN("Error sending route request : %s", znp_strerror(msg->Status));
    else
        LOG_INF("Active endpoint request sent");

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t mt_zdo_permit_join_req_srsp_cb(PermitJoinReqSrspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_WARN("Error permitting new devices to join : %s", znp_strerror(msg->Status));
    else
        LOG_INF("New devices are now allowed to join");

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static mtZdoCb_t mt_zdo_cb = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_zdo_active_ep_rsp_cb,
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
    mt_zdo_state_change_ind_cb,
    NULL,
    NULL,
    mt_zdo_beacon_notify_ind_cb,
    NULL,
    mt_zdo_nwk_discovery_srsp_cb,
    NULL,
    NULL,
    mt_zdo_tc_dev_ind_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_zdo_device_annce_srsp_cb,
    mt_zdo_ext_route_disc_srsp_cb,
    NULL,
    mt_zdo_active_ep_req_srsp_cb,
    mt_zdo_permit_join_req_srsp_cb
};

/********************************
 *          API                 *
 *******************************/

void mt_zdo_register_callbacks(void)
{
    zdoRegisterCallbacks(mt_zdo_cb);
}

void mt_zdo_nwk_discovery_req(SyncActionCb cb)
{
    NwkDiscoveryReqFormat_t req;
    uint8_t status;

    LOG_INF("Sending ZDO network discover request");
    sync_action_cb = cb;
    memcpy(req.ScanChannels, &scan_param, 4);
    req.ScanDuration = 5;
    status = zdoNwkDiscoveryReq(&req);
    if (status != 0)
        LOG_ERR("Cannot start ZDO network discovery");
}

void mt_zdo_startup_from_app(SyncActionCb cb)
{
    LOG_INF("Starting ZDO stack");
    sync_action_cb = cb;
    StartupFromAppFormat_t req;
    req.StartDelay = 0;
    zdoStartupFromApp(&req);
}

void mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr))
{
    _zdo_tc_dev_ind_cb = cb;
}

void mt_zdo_device_annce(uint16_t addr, uint64_t uid, SyncActionCb cb)
{
    LOG_INF("Announce gateway (0x%04X - 0x%016X) to network", addr, uid);

    sync_action_cb = cb;
    DeviceAnnceFormat_t req;
    req.NWKAddr = addr;
    memcpy(req.IEEEAddr, &uid, sizeof(uid));
    req.Capabilities = DEVICE_ANNCE_CAPABILITIES;
    zdoDeviceAnnce(&req);
}

void mt_zdo_ext_route_disc_request(uint16_t addr, SyncActionCb cb)
{
    ExtRouteDiscFormat_t req;

    LOG_INF("Asking route for device 0x%04X", addr);
    sync_action_cb = cb;

    req.DstAddr = addr;
    req.Options = ROUTE_REQ_OPTIONS;
    req.Radius = ROUTE_REQ_RADIUS;
    zdoExtRouteDisc(&req);
}

void mt_zdo_query_active_endpoints(uint16_t short_addr, SyncActionCb cb)
{
    LOG_INF("Requesting active endpoints for device 0x%04X", short_addr);

    ActiveEpReqFormat_t req;
    sync_action_cb = cb;
    req.DstAddr = 0x0000;
    req.NwkAddrOfInterest = short_addr;
    zdoActiveEpReq(&req);
}

void mt_zdo_register_active_ep_rsp_callback(void (*cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list))
{
    _zdo_active_ep_rsp_cb = cb;
}

void mt_zdo_permit_join(SyncActionCb cb)
{
    LOG_INF("Allowing new devices to join for 32s");
    sync_action_cb = cb;
    MgmtPermitJoinReqFormat_t req;
    req.AddrMode = 0xFF;
    req.DstAddr = 0x0000;
    req.Duration = 0x20;
    req.TCSignificance = 0x00;
    zdoMgmtPermitJoinReq(&req);
}

