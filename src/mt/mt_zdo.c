#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "mt_zdo.h"
#include "rpc.h"
#include "logs.h"
#include "utils.h"

/********************************
 *       Constant data          *
 *******************************/
#define SCAN_ALL_CHANNELS_VALUE             0x07FFF800
#define SCAN_DEFAULT_DURATION               5
#define MT_ZDO_NWK_DISCOVERY_TIMEOUT_MS     50

#define ZDO_DEFAULT_STARTUP_DELAY           0

#define DEVICE_ANNCE_CAPABILITIES           0x8E

#define ROUTE_REQ_OPTIONS                   0x00
#define ROUTE_REQ_RADIUS                    0x05

/* MT ZDO commands */
#define ZDO_NWK_ADDR_REQ                    0x00
#define ZDO_IEEE_ADDR_REQ                   0x01
#define ZDO_NODE_DESC_REQ                   0x02
#define ZDO_POWER_DESC_REQ                  0x03
#define ZDO_SIMPLE_DESC_REQ                 0x04
#define ZDO_ACTIVE_EP_REQ                   0x05
#define ZDO_MATCH_DESC_REQ                  0x06
#define ZDO_COMPLEX_DESC_REQ                0x07
#define ZDO_USER_DESC_REQ                   0x08
#define ZDO_END_DEVICE_ANNCE                0x0A
#define ZDO_USER_DESC_SET                   0x0B
#define ZDO_SERVER_DISC_REQ                 0x0C
#define ZDO_END_DEVICE_BIND_REQ             0x20
#define ZDO_BIND_REQ                        0x21
#define ZDO_UNBIND_REQ                      0x22
#define ZDO_MGMT_NWK_DISC_REQ               0x30
#define ZDO_MGMT_LQI_REQ                    0x31
#define ZDO_MGMT_RTG_REQ                    0x32
#define ZDO_MGMT_BIND_REQ                   0x33
#define ZDO_MGMT_LEAVE_REQ                  0x34
#define ZDO_MGMT_DIRECT_JOIN_REQ            0x35
#define ZDO_MGMT_PERMIT_JOIN_REQ            0x36
#define ZDO_MGMT_NWK_UPDATE_REQ             0x37
#define ZDO_MSC_CB_REGISTER                 0x3E
#define ZDO_MSG_CB_REMOVE                   0x3F
#define ZDO_STARTUP_FROM_APP                0x40
#define ZDO_STARTUP_FROM_APP_EX             0x54
#define ZDO_SET_LINK_KEY                    0x23
#define ZDO_REMOVE_LINK_KEY                 0x24
#define ZDO_GET_LINK_KEY                    0x25
#define ZDO_NWK_DISCOVERY_REQ               0x26
#define ZDO_JOIN_REQ                        0x27
#define ZDO_SET_REJOIN_PARAMETERS           0x26
#define ZDO_SEC_ENTRY_LOOKUP_EXT            0x43
#define ZDO_SEC_DEVICE_REMOVE               0x44
#define ZDO_EXT_ROUTE_DISC                  0x45
#define ZDO_EXT_ROUTE_CHECK                 0x45
#define ZDO_EXT_REMOVE_GROUPE               0x47
#define ZDO_EXT_REMOVE_ALL_GROUP            0x48
#define ZDO_EXT_FIND_ALL_GROUPS_ENDPOINT    0x49
#define ZDO_EXT_FIND_GROUP                  0x4A
#define ZDO_EXT_ADD_GROUP                   0x4B
#define ZDO_EXT_COUNT_ALL_GROUPS            0x4C
#define ZDO_EXTRX_IDLE                      0x4D
#define ZDO_EXT_UPDATE_NETWORK_KEY          0x4E
#define ZDO_EXT_SWITCH_NWK_KEY              0x4F
#define ZDO_EXT_NWK_INFO                    0x50
#define ZDO_EXT_SEC_APS_REMOVE_REQ          0x51
#define ZDO_FORCE_CONCENTRATOR_CHANGE       0x52
#define ZDO_EXT_SET_PARAMS                  0x53
#define ZDO_NWK_ADDR_OF_INTEREST            0x29

/* MT ZDO callbacks */
#define ZDO_NWK_ADDR_RSP                    0x80
#define ZDO_IEEE_ADDR_RSP                   0x81
#define ZDO_NODE_DESC_RSP                   0x82
#define ZDO_POWER_DESC_RSP                  0x83
#define ZDO_SIMPLE_DESC_RSP                 0x84
#define ZDO_ACTIVE_EP_RSP                   0x85
#define ZDO_MATCH_DESC_RSP                  0x86
#define ZDO_COMPLEX_DESC_RSP                0x87
#define ZDO_USER_DESC_RSP                   0x88
#define ZDO_USER_DESC_CONF                  0x89
#define ZDO_SERVER_DISC_RSP                 0x8A
#define ZDO_END_DEVICE_BIND_RSP             0xA0
#define ZDO_BIND_RSP                        0xA1
#define ZDO_UNBIND_RSP                      0xA2
#define ZDO_MGMT_NWK_DISC_RSP               0xB0
#define ZDO_MGMT_LQI_RSP                    0xB1
#define ZDO_MGMT_RTG_RSP                    0xB2
#define ZDO_MGMT_BIND_RSP                   0xB3
#define ZDO_MGMT_LEAVE_RSP                  0xB4
#define ZDO_MGMT_DIRECT_JOIN_RSP            0xB5
#define ZDO_MGMT_PERMIT_JOIN_RSP            0xB6
#define ZDO_STATE_CHANGE_IND                0xC0
#define ZDO_END_DEVICE_ANNCE_JOIN           0xC1
#define ZDO_MATCH_DESC_RSP_SENT             0xC2
#define ZDO_STATUS_ERROR_RSP                0xC3
#define ZDO_SRC_RTG_IND                     0xC4
#define ZDO_BEACON_NOTIFY_IND               0xC5
#define ZDO_JOIN_CNF                        0xC6
#define ZDO_NWK_DISCOVERY_CNF               0xC7
#define ZDO_LEAVE_IND                       0xC9
#define ZDO_MSG_CB_INCOMING                 0xFF
#define ZDO_TC_DEV_IND                      0xCA
#define ZDO_PERMIT_JOIN_IND                 0xCB

/* MT ZDO callbacks */

/********************************
 *        Data structures       *
 *******************************/

typedef struct __attribute__((packed))
{
    uint16_t src_addr;
    uint16_t pan_id;
    uint8_t channel;
    uint8_t permit_joining;
    uint8_t router_cap;
    uint8_t device_cap;
    uint8_t protocol_ver;
    uint8_t stack_profile;
    uint8_t lqi;
    uint8_t depth;
    uint8_t update_id;
    uint64_t ext_pan_id;
} beacon_data_t;

/********************************
 *       Local variables        *
 *******************************/

static int _log_domain = -1;
static SyncActionCb sync_action_cb = NULL;
static void (*_zdo_tc_dev_ind_cb)(uint16_t addr, uint64_t ext_addr) = NULL;
static void (*_zdo_active_ep_rsp_cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list) = NULL;
static void (*_zdo_simple_desc_rsp_cb)(uint8_t endpoint, uint16_t profile, uint16_t deviceId) = NULL;

/********************************
 *     MT ZDO callbacks         *
 *******************************/

/* MT ZDO SRSP callbacks */
static uint8_t _nwk_discovery_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_NWK_DISCOVERY_REQ SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            ERR("Error sending ZDO discovery request : %s", zg_logs_znp_strerror(status));
        else
            INF("ZDO discovery request sent");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _end_device_annce_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_END_DEVICE_ANNCE SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            WRN("Error sending device announce request : %s", zg_logs_znp_strerror(status));
        else
            INF("Device announce request sent");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t _ext_route_disc_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_EXT_ROUTE_DISC SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            WRN("Error sending route request : %s", zg_logs_znp_strerror(status));
        else
            INF("Route request sent");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t _active_ep_req_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_ACTIVE_EP_REQ SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            WRN("Error sending active endpoints request : %s", zg_logs_znp_strerror(status));
        else
            INF("Active endpoints request sent");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static uint8_t _permit_join_req_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_MGMT_PERMIT_JOIN_REQ SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            WRN("Error sending permit join request : %s", zg_logs_znp_strerror(status));
        else
            INF("Permit join request sent");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

/* MT ZDO AREQ callbacks */

static uint8_t _active_ep_rsp_cb(ZgMtMsg *msg)
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t active_ep_count;
    uint8_t *active_ep_list;
    uint8_t index = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_ACTIVE_EP_RSP data");
    }
    else
    {
        status = msg->data[2];
        if(status != ZSUCCESS)
        {
            ERR("Error on ACTIVE ENDPOINTS RSP : %s", zg_logs_znp_strerror(status));
            return 1;
        }
        memcpy(&src_addr, msg->data + index, sizeof(src_addr));
        index += sizeof(src_addr);
        memcpy(&status, msg->data + index, sizeof(status));
        index += sizeof(status);
        memcpy(&nwk_addr, msg->data + index, sizeof(nwk_addr));
        index += sizeof(nwk_addr);
        memcpy(&active_ep_count, msg->data + index, sizeof(active_ep_count));
        index += sizeof(active_ep_count);
        active_ep_list = calloc(active_ep_count, sizeof(uint8_t));
        if(!active_ep_list)
        {
            CRI("Cannot allocate memory to retrieve endpoints list");
            return 1;
        }
        memcpy(active_ep_list, msg->data + index, active_ep_count);
        index += active_ep_count;
        INF("MT_ZDO_ACTIVE_EP_RSP received");
        if(_zdo_active_ep_rsp_cb)
            _zdo_active_ep_rsp_cb(nwk_addr, active_ep_count, active_ep_list);
        ZG_VAR_FREE(active_ep_list);
    }

    return 0;
}

static uint8_t _simple_desc_rsp_cb(ZgMtMsg *msg)
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t len;
    uint8_t endpoint;
    uint16_t profile;
    uint16_t device_id;
    uint8_t device_ver;
    uint8_t in_clusters_num;
    uint8_t *in_clusters_list;
    uint8_t out_clusters_num;
    uint8_t *out_clusters_list;
    uint8_t index = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_ACTIVE_EP_RSP data");
    }
    else
    {
        status = msg->data[2];
        if(status != ZSUCCESS)
        {
            ERR("Error on ACTIVE ENDPOINTS RSP : %s", zg_logs_znp_strerror(status));
            return 1;
        }
        memcpy(&src_addr, msg->data + index, sizeof(src_addr));
        index += sizeof(src_addr);
        memcpy(&status, msg->data + index, sizeof(status));
        index += sizeof(status);
        memcpy(&nwk_addr, msg->data + index, sizeof(nwk_addr));
        index += sizeof(nwk_addr);
        memcpy(&len, msg->data + index, sizeof(len));
        index += sizeof(len);
        memcpy(&endpoint, msg->data + index, sizeof(endpoint));
        index += sizeof(endpoint);
        memcpy(&profile, msg->data + index, sizeof(profile));
        index += sizeof(profile);
        memcpy(&device_id, msg->data + index, sizeof(device_id));
        index += sizeof(device_id);
        memcpy(&device_ver, msg->data + index, sizeof(device_ver));
        index += sizeof(device_ver);
        memcpy(&in_clusters_num, msg->data + index, sizeof(in_clusters_num));
        index += sizeof(in_clusters_num);
        in_clusters_list = calloc(in_clusters_num, sizeof(uint8_t));
        if(!in_clusters_list)
        {
            CRI("Cannot allocate memory to retrieve input clusters list");
            return 1;
        }
        memcpy(in_clusters_list, msg->data + index, in_clusters_num);
        index += in_clusters_num;
        memcpy(&out_clusters_num, msg->data + index, sizeof(out_clusters_num));
        index += sizeof(out_clusters_num);
        out_clusters_list = calloc(out_clusters_num, sizeof(uint8_t));
        if(!out_clusters_list)
        {
            CRI("Cannot allocate memory to retrieve output clusters list");
            ZG_VAR_FREE(in_clusters_list);
            return 1;
        }
        memcpy(out_clusters_list, msg->data + index, out_clusters_num);
        INF("MT_ZDO_SIMPLE_DESC_RSP received");
        if(_zdo_simple_desc_rsp_cb)
            _zdo_simple_desc_rsp_cb(endpoint, profile, device_id);
        ZG_VAR_FREE(in_clusters_list);
        ZG_VAR_FREE(out_clusters_list);
    }

    return 0;
}


static uint8_t _state_change_ind_cb(ZgMtMsg *msg)
{
    uint8_t state = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_STATE_CHANGE_IND data");
    }
    else
    {
        state = msg->data[0];
        INF("New ZDO state : 0x%02X", state);
        if(state == 0x09 && sync_action_cb)
            sync_action_cb();
    }

    return 0;
}

static uint8_t _beacon_notify_ind_cb(ZgMtMsg *msg)
{
    uint8_t beacon_count;
    beacon_data_t *beacons;
    uint8_t index = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_BEACON_NOTIFY_IND data");
    }
    else
    {
        beacon_count = msg->data[0];
        beacons = calloc(beacon_count, sizeof(beacon_data_t));
        if(!beacons)
        {
            CRI("Cannot allocate memory to retrieve beacons data");
            return 1;
        }

        for(index = 0; index < beacon_count; index ++)
        {
            memcpy(beacons + (index * sizeof(beacon_data_t)), msg->data + (index * sizeof(beacon_data_t)), sizeof(beacon_data_t));
            INF("=== New visible device in range ===");
            INF("Addr : 0x%04X - PAN : 0x%04X - Channel : 0x%02X",
                    beacons[index].src_addr, beacons[index].pan_id, beacons[index].channel);
            INF("===================================");
        }
    }
    return 0;
}

static uint8_t _tc_dev_ind_cb(ZgMtMsg *msg)
{
    uint16_t src_nwk_addr;
    uint64_t ieee_addr;
    uint16_t parent_addr;
    uint8_t index = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_TC_DEV_IND data");
    }
    else
    {
        memcpy(&src_nwk_addr, msg->data + index, sizeof(src_nwk_addr));
        index += sizeof(src_nwk_addr);
        memcpy(&ieee_addr, msg->data + index, sizeof(ieee_addr));
        index += sizeof(ieee_addr);
        memcpy(&parent_addr, msg->data + index, sizeof(parent_addr));
        index += sizeof(parent_addr);
        INF("============== Device indication ============");
        INF("Addr : 0x%04X - ExtAddr : 0x%016lX - Parent : 0x%04X",
                src_nwk_addr, ieee_addr, parent_addr);
        INF("=============================================");
        if(_zdo_tc_dev_ind_cb)
            _zdo_tc_dev_ind_cb(src_nwk_addr, ieee_addr);
    }
    return 0;
}

uint8_t _permit_join_rsp_cb(ZgMtMsg *msg)
{
    uint16_t addr;
    uint8_t status;
    if(!msg||!msg->data)
    {
        WRN("Cannot extract ZDO_MGMT_PERMIT_JOIN_RSP data");
    }
    else
    {
        memcpy(&addr, msg->data, sizeof(addr));
        status = msg->data[2];
        if(status == 0 && addr == 0x0000)
        {
            INF("Gateway has opened network for new devices to join");
        }
        else if(status && addr != 0x0000)
        {
            INF("Router 0x%04X has opened network for new devices to join", addr);
        }
        else
        {
            WRN("Error on ZDO_MGMT_PERMIT_JOIN_RSP");
        }
    }
    return 0;
}

/* General MT ZDO frames processing callbacks */

static void _process_mt_zdo_srsp(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case ZDO_NWK_DISCOVERY_REQ:
            _nwk_discovery_srsp_cb(msg);
            break;
        case ZDO_END_DEVICE_ANNCE:
            _end_device_annce_srsp_cb(msg);
            break;
        case ZDO_EXT_ROUTE_DISC:
            _ext_route_disc_srsp_cb(msg);
            break;
        case ZDO_ACTIVE_EP_REQ:
            _active_ep_req_srsp_cb(msg);
            break;
        case ZDO_MGMT_PERMIT_JOIN_REQ:
            _permit_join_req_srsp_cb(msg);
            break;
        default:
            WRN("Unknown SRSP command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_zdo_areq(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case ZDO_ACTIVE_EP_RSP:
            _active_ep_rsp_cb(msg);
            break;
        case ZDO_SIMPLE_DESC_RSP:
            _simple_desc_rsp_cb(msg);
            break;
        case ZDO_STATE_CHANGE_IND:
            _state_change_ind_cb(msg);
            break;
        case ZDO_BEACON_NOTIFY_IND:
            _beacon_notify_ind_cb(msg);
            break;
        case ZDO_TC_DEV_IND:
            _tc_dev_ind_cb(msg);
            break;
        case ZDO_MGMT_PERMIT_JOIN_RSP:
            _permit_join_rsp_cb(msg);
            break;
        default:
            WRN("Unknown AREQ command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_zdo_cb(ZgMtMsg *msg)
{
    switch(msg->type)
    {
        case ZG_MT_CMD_AREQ:
            DBG("Received ZDO AREQ request");
            _process_mt_zdo_areq(msg);
            break;
        case ZG_MT_CMD_SRSP:
            DBG("Received ZDO SRSP response");
            _process_mt_zdo_srsp(msg);
            break;
        default:
            WRN("Unsupported MT ZDO message type 0x%02X", msg->type);
            break;
    }
}


/********************************
 *          API                 *
 *******************************/

int zg_mt_zdo_init(void)
{
    _log_domain = zg_logs_domain_register("zg_mt_zdo", ZG_COLOR_CYAN);
    zg_rpc_subsys_cb_set(ZG_MT_SUBSYS_ZDO, _process_mt_zdo_cb);
    INF("MT ZDO module initialized");
    return 0;
}

void zg_mt_zdo_shutdown(void)
{
    INF("MT ZDO module shut down");
}
void zg_mt_zdo_nwk_discovery_req(SyncActionCb cb)
{
    ZgMtMsg msg;
    uint32_t scan_param = SCAN_ALL_CHANNELS_VALUE;
    uint8_t scan_duration = SCAN_DEFAULT_DURATION;
    uint8_t *buffer = NULL;

    INF("Sending ZDO network discover request (%d s)", scan_duration);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_NWK_DISCOVERY_REQ;
    msg.len = sizeof(scan_param) + sizeof(scan_duration);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send ZDO_NWK_DISCOVERY_REQ");
        return;
    }
    memcpy(buffer, &scan_param, sizeof(scan_param));
    memcpy(buffer + sizeof(scan_param), &scan_duration, sizeof(scan_duration));
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_zdo_startup_from_app(SyncActionCb cb)
{
    ZgMtMsg msg;
    uint16_t startup_delay = ZDO_DEFAULT_STARTUP_DELAY;

    INF("Starting ZDO stack");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_STARTUP_FROM_APP;
    msg.len = sizeof(startup_delay);
    msg.data = (uint8_t *)&startup_delay;
    zg_rpc_write(&msg);
}

void zg_mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr))
{
    _zdo_tc_dev_ind_cb = cb;
}

void zg_mt_zdo_device_annce(uint16_t addr, uint64_t uid, SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t *buffer;
    uint8_t cap = DEVICE_ANNCE_CAPABILITIES;

    INF("Announce gateway (0x%04X - 0x%016lX) to network", addr, uid);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_END_DEVICE_ANNCE;
    msg.len = sizeof(addr) + sizeof(uid) + sizeof(cap);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send ZDO_END_DEVICE_ANNCE");
        return;
    }
    memcpy(buffer, &addr, sizeof(addr));
    memcpy(buffer + sizeof(addr), &uid, sizeof(uid));
    memcpy(buffer + sizeof(addr) + sizeof(uid), &cap, sizeof(cap));
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_zdo_ext_route_disc_request(uint16_t addr, SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t options = ROUTE_REQ_OPTIONS;
    uint8_t radius = ROUTE_REQ_RADIUS;
    uint8_t *buffer = NULL;

    INF("Asking route for device 0x%04X", addr);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_EXT_ROUTE_DISC;
    msg.len = sizeof(addr) + sizeof(options) + sizeof(radius);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send ZDO_EXT_ROUTE_DISC");
        return;
    }
    memcpy(buffer, &addr, sizeof(addr));
    memcpy(buffer + sizeof(addr), &options, sizeof(options));
    memcpy(buffer + sizeof(addr) + sizeof(options), &radius, sizeof(radius));
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_zdo_query_active_endpoints(uint16_t short_addr, SyncActionCb cb)
{
    ZgMtMsg msg;
    uint16_t src_addr;
    uint8_t *buffer = NULL;

    INF("Requesting active endpoints for device 0x%04X", short_addr);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_ACTIVE_EP_REQ;
    msg.len = sizeof(short_addr) + sizeof(src_addr);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send ZDO_ACTIVE_EP_REQ");
        return;
    }
    memcpy(buffer, &src_addr, sizeof(src_addr));
    memcpy(buffer + sizeof(src_addr), &short_addr, sizeof(short_addr));
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);

}

void zg_mt_zdo_register_active_ep_rsp_callback(void (*cb)(uint16_t short_addr, uint8_t nb_ep, uint8_t *ep_list))
{
    _zdo_active_ep_rsp_cb = cb;
}

void zg_mt_zdo_register_simple_desc_rsp_cb(void (*cb)(uint8_t endpoint, uint16_t profile, uint16_t device_id))
{
    _zdo_simple_desc_rsp_cb = cb;
}

void zg_mt_zdo_permit_join(SyncActionCb cb)
{
    uint8_t addr_mode = 0x02;
    uint16_t dst_addr = 0x0000;
    uint8_t duration = 0x20;
    uint8_t tcsign = 0x00;
    ZgMtMsg msg;
    uint8_t *buffer = NULL;
    uint8_t index = 0;

    INF("Allowing new devices to join for 32s");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_ZDO;
    msg.cmd = ZDO_MGMT_PERMIT_JOIN_REQ;
    msg.len = sizeof(addr_mode) + sizeof(dst_addr) + sizeof(duration) + sizeof(tcsign);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send ZDO_ACTIVE_EP_REQ");
        return;
    }
    memcpy(buffer + index, &addr_mode, sizeof(addr_mode));
    index += sizeof(addr_mode);
    memcpy(buffer + index, &dst_addr, sizeof(dst_addr));
    index += sizeof(dst_addr);
    memcpy(buffer + index, &duration, sizeof(duration));
    index += sizeof(duration);
    memcpy(buffer + index, &tcsign, sizeof(tcsign));
    index += sizeof(tcsign);
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

