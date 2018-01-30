#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "mt_sys.h"
#include "keys.h"
#include "logs.h"
#include "rpc.h"
#include "utils.h"

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

/* MT SYS commands */
#define SYS_RESET_REQ               0x00
#define SYS_PING                    0x01
#define SYS_VERSION                 0x02
#define SYS_SET_EXTADDR             0x03
#define SYS_GET_EXTADRR             0x04
#define SYS_RAM_READ                0x05
#define SYS_RAM_WRITE               0x06
#define SYS_OSAL_NV_READ            0x08
#define SYS_OSAL_NV_WRITE           0x09
#define SYS_OSAL_NV_ITEM_INIT       0x07
#define SYS_OSAL_NV_DELETE          0x12
#define SYS_OSAL_NV_LENGTH          0x13
#define SYS_OSAL_START_TIMER        0x0A
#define SYS_OSAL_STOP_TIMER         0x0B
#define SYS_RANDOM                  0x0C
#define SYS_ADC_READ                0x0D
#define SYS_GPIO                    0x0E
#define SYS_STACK_TUNE              0x0F
#define SYS_SET_TIME                0x10
#define SYS_GET_TIME                0x11
#define SYS_SET_TX_POWER            0x14
#define SYS_ZDIAGS_INIT_STATS       0x17
#define SYS_ZDIAGS_CLEAR_STATS      0x18
#define SYS_ZDIAGS_GET_STATS        0x19
#define SYS_ZDIAGS_RESTORE_STATS_NV 0x1A
#define SYS_ZDIAGS_SAVE_STATS_TO_NV 0x1B
#define SYS_NV_CREATE               0x30
#define SYS_NV_DELETE               0x31
#define SYS_NV_LENGTH               0x32
#define SYS_NV_READ                 0x33
#define SYS_NV_WRITE                0x34
#define SYS_NV_UPDATE               0x35
#define SYS_NV_COMPACT              0x36
#define SYS_OSAL_NV_READ_EXT        0x08
#define SYS_OSAL_NV_WRITE_EXT       0x09

/* MT SYS callbacks */
#define SYS_RESET_IND               0x80
#define SYS_OSAL_TIMER_EXPIRED      0x81

/********************************
 *       Local variables        *
 *******************************/

static int _log_domain = -1;
static int _init_count = 0;

/* Callback set for any synchronous operation (see SREQ in MT specification) */
static SyncActionCb sync_action_cb = NULL;
static uint64_t _ext_addr = 0x0000000000000000;

/********************************
 *     MT SYS callbacks         *
 *******************************/

/* SRSP callbacks */

static uint8_t _ping_srsp_cb(ZgMtMsg *msg)
{
    if(msg && msg->data && msg->len >= 1)
        INF("System ping SRSP received. Capabilities : %04X", msg->data[0]);
    else
        WRN("Cannot extract PING SRSP data");
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _get_ext_addr_srsp_cb(ZgMtMsg *msg)
{
    if(!msg || !msg->data)
    {
        WRN("Cannot extract EXT_ADDR_SRSP data");
    }
    else
    {
        memcpy(&_ext_addr, msg->data, sizeof(_ext_addr));
        INF("Received extended address (0x%016lX)", _ext_addr);
    }

    if(sync_action_cb)
        sync_action_cb();
    return 0;
}

static uint8_t _osal_nv_write_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract SYS_OSAL_NV_WRITE data");
    }
    else
    {
        status = msg->data[0];
        if(status)
            ERR("SYS_OSAL_NV_WRITE failed with status 0x%02X", status);
        else
            INF("SYS_OSAL_NV_WRITE OK");
    }

    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

/* AREQ callbacks */

static uint8_t _reset_ind_cb(ZgMtMsg *msg)
{
    if(!msg||!msg->data)
    {
        WRN("Cannot extract SYS_RESET_IND data");
    }
    else
    {
        INF("System reset ind. received. Reason : %s ",
                msg->data[0] == 0 ? "power up":(msg->data[0] == 1 ?
                    "External":"Watchdog"));
        INF("Transport version : %d", msg->data[1]);
        INF("Product ID : %d", msg->data[2]);
        INF("ZNP version : %d.%d.%d", msg->data[3], msg->data[4], msg->data[5]);
    }
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}


/* General MT SYS frames processing callbacks */

static void _process_mt_sys_sreq(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        default:
            WRN("Unknown SREQ command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_sys_srsp(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case SYS_GET_EXTADRR:
            _get_ext_addr_srsp_cb(msg);
            break;
        case SYS_PING:
            _ping_srsp_cb(msg);
            break;
        case SYS_OSAL_NV_WRITE:
            _osal_nv_write_srsp_cb(msg);
            break;
        default:
            WRN("Unknown SRSP command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_sys_areq(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case SYS_RESET_IND:
            _reset_ind_cb(msg);
            break;
        default:
            WRN("Unknown AREQ command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_sys_cb(ZgMtMsg *msg)
{
    switch(msg->type)
    {
        case ZG_MT_CMD_SREQ:
            DBG("Received SYS SREQ request");
            _process_mt_sys_sreq(msg);
            break;
        case ZG_MT_CMD_AREQ:
            DBG("Received SYS AREQ request");
            _process_mt_sys_areq(msg);
            break;
        case ZG_MT_CMD_SRSP:
            DBG("Received SYS SRSP response");
            _process_mt_sys_srsp(msg);
            break;
        default:
            WRN("Unsupported MT SYS message type 0x%02X", msg->type);
            break;
    }
}

/********************************
 *          Internals           *
 *******************************/

static void _sys_osal_nv_write(uint16_t id, uint8_t offset, uint8_t length, uint8_t *data)
{
    if(!data)
    {
        ERR("Cannot write OSAL NV element : data is empty");
        return;
    }

    ZgMtMsg msg;
    uint8_t *buffer = NULL;

    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_SYS;
    msg.cmd = SYS_OSAL_NV_WRITE;
    msg.len = sizeof(id) + sizeof(offset) + sizeof(length) + length;
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        ERR("Cannot allocate memory for SYS_OSAL_NV_WRITE");
        return;
    }
    memcpy(buffer, &id, sizeof(id));
    memcpy(buffer + sizeof(id), &offset, sizeof(offset));
    memcpy(buffer + sizeof(id) + sizeof(offset), &length, sizeof(length));
    memcpy(buffer + sizeof(id) + sizeof(offset) + sizeof(length),
            data, length);
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(msg.data);
}

/********************************
 *          API                 *
 *******************************/

int zg_mt_sys_init(void)
{
    ENSURE_SINGLE_INIT(_init_count);
    _log_domain = zg_logs_domain_register("zg_mt_sys", ZG_COLOR_LIGHTRED);
    zg_rpc_init();
    zg_rpc_subsys_cb_set(ZG_MT_SUBSYS_SYS, _process_mt_sys_cb);
    INF("MT SYS module initialized");
    return 0;
}

void zg_mt_sys_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    zg_rpc_shutdown();
    INF("MT SYS module shut down");
}

void zg_mt_sys_reset_dongle (SyncActionCb cb)
{
    ZgMtMsg msg;
    uint8_t reset_type = 1;

    INF("Resetting ZNP");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_SYS;
    msg.cmd = SYS_RESET_REQ;
    msg.data = &reset_type; /* Ask for a soft reset to avoid dealing with USB re-enumeration */
    msg.len = sizeof(reset_type);
    zg_rpc_write(&msg);
}

void zg_mt_sys_ping(SyncActionCb cb)
{
    ZgMtMsg msg;

    INF("Ping dongle");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_SYS;
    msg.cmd = SYS_PING;
    msg.len = 0;
    zg_rpc_write(&msg);
}

void zg_mt_sys_nv_write_startup_options(MtSysStartupOptions options,SyncActionCb cb)
{
    uint8_t nv_options = 0x00;

    INF("Writing startup options");
    sync_action_cb = cb;

    if(options & STARTUP_CLEAR_NWK_FRAME_COUNTER)
        nv_options |= 0x1 << 7;
    if(options & STARTUP_CLEAR_STATE)
        nv_options |= 0x1 << 1;
    if(options & STARTUP_CLEAR_CONFIG)
        nv_options |= 0x1;

    _sys_osal_nv_write(3, 0, 1, &nv_options);
}

void zg_mt_sys_nv_write_coord_flag(SyncActionCb cb)
{
    uint8_t nv_coord_data[] = {0};

    INF("Setting device as coordinator");
    sync_action_cb = cb;
    _sys_osal_nv_write(0x87, 0, 1, nv_coord_data);
}

void zg_mt_sys_nv_write_disable_security(SyncActionCb cb)
{
    uint8_t nv_disable_sec_data[] = {0};

    INF("Disabling NWK security");
    sync_action_cb = cb;
    _sys_osal_nv_write(0x64, 0, 1, nv_disable_sec_data);
}

void zg_mt_sys_nv_write_enable_security(SyncActionCb cb)
{
    uint8_t nv_enable_sec_data[] = {1};

    INF("Enabling NWK security");
    sync_action_cb = cb;
    _sys_osal_nv_write(0x64, 0, 1, nv_enable_sec_data);
}

void zg_mt_sys_nv_set_pan_id(SyncActionCb cb)
{
    uint8_t nv_set_pan_id[] = {0xCD, 0xAB};

    INF("Setting PAN ID");
    sync_action_cb = cb;
    _sys_osal_nv_write(0x83, 0, 2, nv_set_pan_id);
}

void zg_mt_sys_nv_write_nwk_key(SyncActionCb cb)
{
    INF("Setting network key");

    sync_action_cb = cb;
    _sys_osal_nv_write(0x62, 0, zg_keys_network_key_size_get(), zg_keys_network_key_get());
}

void zg_mt_sys_check_ext_addr(SyncActionCb cb)
{
    ZgMtMsg msg;
    INF("Retrieving extended address");

    sync_action_cb = cb;
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SREQ;
    msg.subsys = ZG_MT_SUBSYS_SYS;
    msg.cmd = SYS_GET_EXTADRR;
    msg.len = 0;
    zg_rpc_write(&msg);
}

uint64_t zg_mt_sys_get_ext_addr(void)
{
    return _ext_addr;
}

void zg_mt_sys_nv_write_channel(uint8_t channel, SyncActionCb cb)
{
    uint32_t channel_mask = 0x0000;

    switch(channel)
    {
        case 11:
            channel_mask = 0x00000800;
            break;
        case 12:
            channel_mask = 0x00001000;
            break;
        case 13:
            channel_mask = 0x00002000;
            break;
        case 14:
            channel_mask = 0x00004000;
            break;
        case 15:
            channel_mask = 0x00008000;
            break;
        case 16:
            channel_mask = 0x00010000;
            break;
        case 17:
            channel_mask = 0x00020000;
            break;
        case 18:
            channel_mask = 0x00040000;
            break;
        case 19:
            channel_mask = 0x00080000;
            break;
        case 20:
            channel_mask = 0x00100000;
            break;
        case 21:
            channel_mask = 0x00200000;
            break;
        case 22:
            channel_mask = 0x00400000;
            break;
        case 23:
            channel_mask = 0x00800000;
            break;
        case 24:
            channel_mask = 0x01000000;
            break;
        case 25:
            channel_mask = 0x02000000;
            break;
        case 26:
            channel_mask = 0x04000000;
            break;
        default:
            ERR("Cannot set channel %d (not a proper channel value)", channel);
            if(cb)
                cb();
            return;
            break;
    }
    INF("Setting radio to operate on channel %d", channel);
    sync_action_cb = cb;
    _sys_osal_nv_write(0x84, 0, sizeof(channel_mask), (uint8_t *)&channel_mask);
}
