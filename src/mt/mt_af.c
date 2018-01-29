#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "mt_af.h"
#include "rpc.h"
#include "logs.h"
#include "utils.h"

/********************************
 *       Constant data          *
 *******************************/

#define DATA_REQUEST_DEFAULT_OPTIONS        0x00
#define DATA_REQUEST_DEFAULT_RADIUS         0x5
#define DEFAULT_LATENCY                     0

/* Inter-Pan commands */
#define INTER_PAN_CLEAR                     0x00
#define INTER_PAN_SET                       0x01
#define INTER_PAN_REGISTER                  0x02
#define INTER_PAN_CHK                       0x03

/* MT AF commands */
#define AF_REGISTER                         0x00
#define AF_DATA_REQUEST                     0x01
#define AF_DATA_REQUEST_EXT                 0x02
#define AF_DATA_REQUEST_SRC_RTG             0x03
#define AF_INTER_PAN_CTL                    0x10
#define AF_DATA_STORE                       0x11
#define AF_DATA_RETRIEVE                    0x12
#define AF_APSF_CONFIG_SET                  0x13

/* MT AF callbacks */
#define AF_DATA_CONFIRM                     0x80
#define AF_REFLECT_ERROR                    0x83
#define AF_INCOMING_MSG                     0x81
#define AF_INCOMING_MSG_EXT                 0x82

/********************************
 *          Data Types          *
 *******************************/

typedef enum
{
    SHORT_ADDR_MODE = 2,
    EXT_ADDR_MODE = 3,
}DstAddrMode;

/********************************
 *       Static variables       *
 *******************************/

static AfIncomingMessageCb _af_incoming_msg_cb = NULL;
static SyncActionCb sync_action_cb = NULL;
static uint8_t _transaction_id = 0;
static int _log_domain = -1;


/********************************
 *      MT AF callbacks         *
 *******************************/

/* MT AF SRSP callbacks */

static uint8_t _register_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status = 0;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_REGISTER SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            ERR("Error registering new enpdoint : %s", zg_logs_znp_strerror(status));
        else
            INF("New endpoint registered");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _data_request_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status = 0;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_DATA_REQUEST SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
        {
            ERR("Error sending data to remote device : %s", zg_logs_znp_strerror(status));
        }
        else
        {
            INF("Data request sent to remote device");
            _transaction_id++;
        }
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _data_request_ext_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status = 0;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_DATA_REQUEST_EXT SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
        {
            ERR("Error sending extending data request to remote device : %s", zg_logs_znp_strerror(status));
        }
        else
        {
            INF("Extended data request sent to remote device");
            _transaction_id++;
        }
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _inter_pan_ctl_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status = 0;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_INTER_PAN_CTL SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != ZSUCCESS)
            ERR("Error sending interpan control command : %s", zg_logs_znp_strerror(status));
        else
            INF("Inter-pan command applied");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

/* MT AF AREQ callbacks */

static uint8_t _incoming_msg_cb(ZgMtMsg *msg)
{
    uint16_t group_id;
    uint16_t cluster;
    uint64_t src_addr;
    uint8_t src_endpoint;
    uint8_t dst_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t sec;
    uint32_t timestamp;
    uint8_t trans;
    uint8_t len;
    uint8_t index = 0;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_INCOMING_MSG AREQ data");
    }
    else
    {
        memcpy(&group_id, msg->data + index, sizeof(group_id));
        index += sizeof(group_id);
        memcpy(&cluster, msg->data + index, sizeof(cluster));
        index += sizeof(cluster);
        memcpy(&src_addr, msg->data + index, sizeof(src_addr));
        index += sizeof(src_addr);
        memcpy(&src_endpoint, msg->data + index, sizeof(src_endpoint));
        index += sizeof(src_endpoint);
        memcpy(&dst_endpoint, msg->data + index, sizeof(dst_endpoint));
        index += sizeof(dst_endpoint);
        memcpy(&was_broadcast, msg->data + index, sizeof(was_broadcast));
        index += sizeof(was_broadcast);
        memcpy(&link_quality, msg->data + index, sizeof(link_quality));
        index += sizeof(link_quality);
        memcpy(&sec, msg->data + index, sizeof(sec));
        index += sizeof(sec);
        memcpy(&timestamp, msg->data + index, sizeof(timestamp));
        index += sizeof(timestamp);
        memcpy(&trans, msg->data + index, sizeof(trans));
        index += sizeof(trans);
        memcpy(&len, msg->data + index, sizeof(len));
        index += sizeof(len);
        INF("Extended AF message received");
        INF("Group id : 0x%04X", group_id);
        INF("Cluster id : 0x%04X", cluster);
        INF("Source addr : 0x%016lX", src_addr);
        INF("Source Endpoint : 0x%02X", src_endpoint);
        INF("Dest Endpoint : 0x%02X", dst_endpoint);
        INF("Was Broadcast : 0x%02X", was_broadcast);
        INF("Link Quality : 0x%02X", link_quality);
        INF("Security Use : 0x%02X", sec);
        INF("Timestamp : 0x%08X", timestamp);
        INF("Transaction sequence num : 0x%02X", trans);
        INF("Length : %d", len);

        if(_af_incoming_msg_cb)
            _af_incoming_msg_cb(src_addr, dst_endpoint, cluster, msg->data + index, len);
   }

    return 0;
}

static uint8_t _incoming_msg_ext_cb(ZgMtMsg *msg)
{
    uint16_t group_id;
    uint16_t cluster;
    uint8_t src_addr_mode;
    uint64_t src_addr;
    uint8_t src_endpoint;
    uint16_t pan_id;
    uint8_t dst_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t sec;
    uint32_t timestamp;
    uint8_t trans;
    uint8_t len;
    uint8_t index = 0;

    if(!msg || !msg->data)
    {
        WRN("Cannot extract AF_INCOMING_MSG AREQ data");
    }
    else
    {
        memcpy(&group_id, msg->data + index, sizeof(group_id));
        index += sizeof(group_id);
        memcpy(&cluster, msg->data + index, sizeof(cluster));
        index += sizeof(cluster);
        memcpy(&src_addr_mode, msg->data + index, sizeof(src_addr_mode));
        index += sizeof(src_addr_mode);
        memcpy(&src_addr, msg->data + index, sizeof(src_addr));
        index += sizeof(src_addr);
        memcpy(&src_endpoint, msg->data + index, sizeof(src_endpoint));
        index += sizeof(src_endpoint);
        memcpy(&pan_id, msg->data + index, sizeof(pan_id));
        index += sizeof(pan_id);
        memcpy(&dst_endpoint, msg->data + index, sizeof(dst_endpoint));
        index += sizeof(dst_endpoint);
        memcpy(&was_broadcast, msg->data + index, sizeof(was_broadcast));
        index += sizeof(was_broadcast);
        memcpy(&link_quality, msg->data + index, sizeof(link_quality));
        index += sizeof(link_quality);
        memcpy(&sec, msg->data + index, sizeof(sec));
        index += sizeof(sec);
        memcpy(&timestamp, msg->data + index, sizeof(timestamp));
        index += sizeof(timestamp);
        memcpy(&trans, msg->data + index, sizeof(trans));
        index += sizeof(trans);
        memcpy(&len, msg->data + index, sizeof(len));
        index += sizeof(len);
        INF("AF message received");
        INF("Group id : 0x%04X", group_id);
        INF("Cluster id : 0x%04X", cluster);
        INF("Source addr mode : 0x%02X", src_addr_mode);
        INF("Source addr : 0x%016lX", src_addr);
        INF("Source Endpoint : 0x%02X", src_endpoint);
        INF("Source PAN id : 0x%04X", pan_id);
        INF("Dest Endpoint : 0x%02X", dst_endpoint);
        INF("Was Broadcast : 0x%02X", was_broadcast);
        INF("Link Quality : 0x%02X", link_quality);
        INF("Security Use : 0x%02X", sec);
        INF("Timestamp : 0x%08X", timestamp);
        INF("Transaction sequence num : 0x%02X", trans);
        INF("Length : %d", len);
        /* TODO : add parsing for huge buffer, ie with multiple AF_DATA_RETRIEVE
        */
        if(_af_incoming_msg_cb)
            _af_incoming_msg_cb(src_addr, dst_endpoint, cluster, msg->data, len);
    }

    return 0;
}

static uint8_t _data_confirm_cb(ZgMtMsg *msg)
{
    uint8_t status;
    uint8_t endpoint;
    uint8_t trans;
    uint8_t index = 0;

    if(!msg||!msg->data)
    {
        WRN("Cannot extract AF_INCOMING_MSG AREQ data");
    }
    else
    {
        memcpy(&status, msg->data + index, sizeof(status));
        index += sizeof(status);
        memcpy(&endpoint, msg->data + index, sizeof(endpoint));
        index += sizeof(endpoint);
        memcpy(&trans, msg->data + index, sizeof(trans));
        index += sizeof(trans);

        if(status != ZSUCCESS)
        {
            ERR("Data error for transaction 0x%02X - endpoint 0x%02X : %s",
                    endpoint, trans, zg_logs_znp_strerror(status));
        }
        else
        {
            INF("AF_DATA_CONFIRM received for transaction 0x%02X - endpoint 0x%02X", trans, endpoint);
        }
    }
    return 0;
}

/* General MT AF frames processing callbacks */

static void _process_mt_af_srsp(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case AF_REGISTER:
            _register_srsp_cb(msg);
            break;
        case AF_DATA_REQUEST:
            _data_request_srsp_cb(msg);
            break;
        case AF_DATA_REQUEST_EXT:
            _data_request_ext_srsp_cb(msg);
            break;
        case AF_INTER_PAN_CTL:
            _inter_pan_ctl_srsp_cb(msg);
            break;
        default:
            WRN("Unknown SRSP command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_af_areq(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case AF_INCOMING_MSG:
            _incoming_msg_cb(msg);
            break;
        case AF_INCOMING_MSG_EXT:
            _incoming_msg_ext_cb(msg);
            break;
        case AF_DATA_CONFIRM:
            _data_confirm_cb(msg);
            break;
        default:
            WRN("Unknown AREQ command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_af_cb(ZgMtMsg *msg)
{
    switch(msg->type)
    {
        case ZG_MT_CMD_AREQ:
            DBG("Received AF AREQ request");
            _process_mt_af_areq(msg);
            break;
        case ZG_MT_CMD_SRSP:
            DBG("Received AF SRSP response");
            _process_mt_af_srsp(msg);
            break;
        default:
            WRN("Unsupported MT AF message type 0x%02X", msg->type);
            break;
    }
}

/********************************
 *          API                 *
 *******************************/

int zg_mt_af_init(void)
{
    _log_domain = zg_logs_domain_register("zg_mt_af", ZG_COLOR_LIGHTYELLOW);
    zg_rpc_subsys_cb_set(ZG_MT_SUBSYS_AF, _process_mt_af_cb);
    INF("MT AF module initialized");
    return 0;
}

void zg_mt_af_shutdown(void)
{
    INF("MT AF module shut down");
}

void zg_mt_af_register_endpoint(   uint8_t endpoint,
                                uint16_t profile,
                                uint16_t device_id,
                                uint8_t device_ver,
                                uint8_t in_clusters_num,
                                uint16_t *in_clusters_list,
                                uint8_t out_clusters_num,
                                uint16_t *out_clusters_list,
                                SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t index = 0;
    uint8_t *buffer;
    uint8_t default_latency = 0x00;

    INF("Registering new endpoint with profile 0x%4X", profile);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_AF;
    msg.cmd = AF_REGISTER;
    msg.len = sizeof(endpoint) \
              + sizeof (profile) \
              + sizeof (device_id) \
              + sizeof (device_ver) \
              + sizeof(default_latency) \
              + sizeof(in_clusters_num) \
              + (sizeof(*in_clusters_list) * in_clusters_num) \
              + sizeof(out_clusters_num) \
              + (sizeof(*out_clusters_list) * out_clusters_num);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send AF_REGISTER command");
        return;
    }
    memcpy(buffer + index, &endpoint, sizeof(endpoint));
    index += sizeof(endpoint);
    memcpy(buffer + index , &profile, sizeof(profile));
    index += sizeof(profile);
    memcpy(buffer + index , &device_id, sizeof(device_id));
    index += sizeof(device_id);
    memcpy(buffer + index , &device_ver, sizeof(device_ver));
    index += sizeof(device_ver);
    memcpy(buffer + index , &default_latency, sizeof(default_latency));
    index += sizeof(default_latency);
    memcpy(buffer + index , &in_clusters_num, sizeof(in_clusters_num));
    index += sizeof(in_clusters_num);
    memcpy(buffer + index , in_clusters_list, in_clusters_num * sizeof(uint16_t));
    index += in_clusters_num * sizeof(uint16_t);
    memcpy(buffer + index , &out_clusters_num, sizeof(out_clusters_num));
    index += sizeof(out_clusters_num);
    memcpy(buffer + index , out_clusters_list, out_clusters_num * sizeof(uint16_t));
    index += out_clusters_num * sizeof(uint16_t);
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_af_set_inter_pan_endpoint(uint8_t endpoint, SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t interpan_command_data = INTER_PAN_REGISTER;
    uint8_t *buffer;

    INF("Setting inter-pan endpoint 0x%02X", endpoint);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_AF;
    msg.cmd = AF_INTER_PAN_CTL;
    msg.len = sizeof(interpan_command_data + sizeof(endpoint));
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        ERR("Cannot allocate memory for AF_INTER_PAN_CTL command");
        return;
    }
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_af_set_inter_pan_channel(uint8_t channel, SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t interpan_command_data = INTER_PAN_SET;
    uint8_t *buffer = NULL;

    INF("Setting inter-pan channel 0x%02X", channel);
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_AF;
    msg.cmd = AF_INTER_PAN_CTL;
    msg.len = sizeof(interpan_command_data) + sizeof(channel);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory for AF_INTER_PAN_CTL command");
        return;
    }
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}

void zg_mt_af_register_incoming_message_callback(AfIncomingMessageCb cb)
{
    _af_incoming_msg_cb = cb;
}

void zg_mt_af_send_data_request_ext(   uint64_t dst_addr,
                                    uint16_t dst_pan,
                                    uint8_t src_endpoint,
                                    uint8_t dst_endpoint,
                                    uint16_t cluster,
                                    uint16_t len,
                                    void *data,
                                    SyncActionCb cb)
{
    uint8_t addr_mode = SHORT_ADDR_MODE;
    uint8_t options = DATA_REQUEST_DEFAULT_OPTIONS;
    uint8_t radius = DATA_REQUEST_DEFAULT_RADIUS;
    uint8_t *buffer = NULL;
    uint8_t index = 0;
    ZgMtMsg msg;

    if(len == 0 || !data)
    {
        ERR("Cannot send AF_DATA_REQUEST_EXT (%s)", data ? "length is invalid":"no data provided");
        return;
    }

    DBG("Sending AF_DATA_REQUEST_EXT");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_AF;
    msg.cmd = AF_DATA_REQUEST_EXT;
    msg.len = sizeof(addr_mode) \
              + sizeof(dst_addr) \
              + sizeof(dst_endpoint) \
              + sizeof(dst_pan) \
              + sizeof(src_endpoint) \
              + sizeof(cluster) \
              + sizeof(_transaction_id) \
              + sizeof(options) \
              + sizeof(radius) \
              + sizeof(len) \
              + len;
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory to send AF_REGISTER command");
        return;
    }

    memcpy(buffer + index, &addr_mode, sizeof(addr_mode));
    index += sizeof(addr_mode);
    memcpy(buffer + index , &dst_addr, sizeof(dst_addr));
    index += sizeof(dst_addr);
    memcpy(buffer + index , &dst_endpoint, sizeof(dst_endpoint));
    index += sizeof(dst_endpoint);
    memcpy(buffer + index , &dst_pan, sizeof(dst_pan));
    index += sizeof(dst_pan);
    memcpy(buffer + index , &src_endpoint, sizeof(src_endpoint));
    index += sizeof(src_endpoint);
    memcpy(buffer + index , &cluster, sizeof(cluster));
    index += sizeof(cluster);
    memcpy(buffer + index , &_transaction_id, sizeof(_transaction_id));
    index += sizeof(_transaction_id);
    memcpy(buffer + index , &options, sizeof(options));
    index += sizeof(options);
    memcpy(buffer + index , &radius, sizeof(radius));
    index += sizeof(radius);
    memcpy(buffer + index , &len, sizeof(len));
    index += sizeof(len);
    memcpy(buffer + index , data, len);
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}
