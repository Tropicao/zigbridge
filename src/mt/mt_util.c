#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "mt_util.h"
#include "rpc.h"
#include "logs.h"
#include "utils.h"


/********************************
 *       Constant data          *
 *******************************/

/* MT SYS commands */
#define UTIL_GET_DEVICE_INFO                0x00
#define UTIL_GET_NV_INFO                    0x01
#define UTIL_SET_PANID                      0x02
#define UTIL_SET_CHANNELS                   0x03
#define UTIL_SET_SECLEVEL                   0x04
#define UTIL_SET_PRECFGKEY                  0x05
#define UTIL_CALLBACK_SUB_CMD               0x06
#define UTIL_KEY_EVENT                      0x07
#define UTIL_TIME_ALIVE                     0x09
#define UTIL_LED_CONTROL                    0x0A
#define UTIL_LOOPBACK                       0x10
#define UTIL_DATA_REQ                       0x11
#define UTIL_SRC_MATCH_ENABLE               0x20
#define UTIL_SRC_MATCH_ADD_ENTRY            0x21
#define UTIL_SRC_MATCH_DEL_ENTRY            0x22
#define UTIL_SRC_MATCH_CHECK_SRC_ADDR       0x23
#define UTIL_SRC_MATCH_ACK_ALL_PENDING      0x24
#define UTIL_SRC_MATCH_CHECK_ALL_PENDING    0x25
#define UTIL_ADDRMGR_EXT_ADDR_LOOKUP        0x40
#define UTIL_ADDRMGR_NWK_ADDR_LOOKUP        0x41
#define UTIL_APSME_LINK_KEY_DATA_GET        0x44
#define UTIL_APSME_LINK_KEY_NV_ID_GET       0x45
#define UTIL_APSME_REQUEST_KEY_CMD          0x4B
#define UTIL_ASSOC_COUNT                    0x48
#define UTIL_ASSOC_FIND_DEVICE              0x49
#define UTIL_ASSOC_GET_WITH_ADDRESS         0x4A
#define UTIL_BIND_ADD_ENTRY                 0x4D
#define UTIL_ZCL_KEY_EST_INIT_EST           0x80
#define UTIL_ZCL_KEY_EST_SIGN               0x81
#define UTIL_SRNG_GEN                       0x4C

/* MT UTIL callbacks */
#define UTIL_SYNC_REQ                       0xE0
#define UTIL_ZCL_KEY_ESTABLISHED_IND        0xE1


/********************************
 *       Local variables        *
 *******************************/

/* Callback set for any synchronous operation (see SREQ in MT specification) */
static SyncActionCb sync_action_cb = NULL;
static int _log_domain = -1;
static int _init_count = 0;

/********************************
 *     MT UTIL callbacks         *
 *******************************/

/* SRSP callbacks */

static uint8_t _callback_sub_cmd_srsp_cb(ZgMtMsg *msg)
{
    uint8_t status;
    if(!msg || !msg->data)
    {
        WRN("Cannot extract UTIL_CALLBACK_SUB_CMD SRSP data");
    }
    else
    {
        status = msg->data[0];
        if(status != 0)
            ERR("UTIL_CALLBACK_SUB_CMD failed with status %d (%s)", status, zg_logs_znp_strerror(status));
        else
            INF("UTIL_CALLBACK_SUB_CMD OK");
    }
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

/* General MT UTIL frames processing callbacks */

static void _process_mt_util_srsp(ZgMtMsg *msg)
{
    switch(msg->cmd)
    {
        case UTIL_CALLBACK_SUB_CMD:
            _callback_sub_cmd_srsp_cb(msg);
            break;
        default:
            WRN("Unknown SRSP command 0x%02X", msg->cmd);
            break;
    }
}

static void _process_mt_util_cb(ZgMtMsg *msg)
{
    switch(msg->type)
    {
        case ZG_MT_CMD_SRSP:
            DBG("Received UTIL SRSP response");
            _process_mt_util_srsp(msg);
            break;
        default:
            WRN("Unsupported MT UTIL message type 0x%02X", msg->type);
            break;
    }
}

/********************************
 *          API                 *
 *******************************/

int zg_mt_util_init(void)
{
    ENSURE_SINGLE_INIT(_init_count);
    zg_rpc_init();
    _log_domain = zg_logs_domain_register("zg_mt_util", ZG_COLOR_MAGENTA);
    zg_rpc_subsys_cb_set(ZG_MT_SUBSYS_UTIL, _process_mt_util_cb);
    INF("MT UTIL module initialized");
    return 0;
}

void zg_mt_util_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    zg_rpc_shutdown();
    INF("MT UTIL module shut down");
}


void zg_mt_util_af_subscribe_cmd (SyncActionCb cb)
{
    ZgMtMsg msg;
    uint16_t mt_af_subsys = 0x0400;
    uint8_t status = 0x1;
    uint8_t *buffer = NULL;

    INF("Subscribing to MT_AF callbacks");
    sync_action_cb = cb;
    msg.type = ZG_MT_CMD_SRSP;
    msg.subsys = ZG_MT_SUBSYS_UTIL;
    msg.cmd = UTIL_CALLBACK_SUB_CMD;
    msg.len = sizeof(mt_af_subsys)+sizeof(status);
    buffer = calloc(msg.len, sizeof(uint8_t));
    if(!buffer)
    {
        CRI("Cannot allocate memory for UTIL_CALLBACK_SUB_CMD command");
        return;
    }
    msg.data = buffer;
    zg_rpc_write(&msg);
    ZG_VAR_FREE(buffer);
}
