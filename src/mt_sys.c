#include <stdlib.h>
#include "mt_sys.h"
#include "dbgPrint.h"
#include "mtSys.h"
#include "uv.h"
#include "rpc.h"
#include <string.h>

static uint8_t nv_clear_data[] = {3};
static uint8_t nv_coord_data[] = {0};
static uint8_t nv_disable_sec_data[] = {0};
static uint8_t nv_set_pan_id[] = {0xFE, 0xCA};

/* Callback set for any synchronous operation (see SREQ in MT specification) */
static SyncActionCb sync_action_cb = NULL;

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


/********************************
 *     MT SYS callbacks         *
 *******************************/

static uint8_t _ping_srsp_cb(PingSrspFormat_t *msg)
{
    LOG_INF("System ping SRSP received. Capabilities : %04X",
            msg->Capabilities);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _reset_ind_cb(ResetIndFormat_t *msg)
{
    LOG_INF("System reset ind. received. Reason : %s ",
            msg->Reason == 0 ? "power up":(msg->Reason == 1 ?
                "External":"Watchdog"));
    LOG_INF("Transport version : %d", msg->TransportRev);
    LOG_INF("Product ID : %d", msg->ProductId);
    LOG_INF("ZNP version : %d.%d.%d", msg->MajorRel, msg->MinorRel, msg->HwRev);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t _osal_nv_write_srsp_cb(OsalNvWriteSrspFormat_t *msg)
{
    if(msg->Status == 0)
        LOG_INF("NV write status : %02X", msg->Status);
    else
        LOG_ERR("NV write status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}


static mtSysCb_t mt_sys_cb = {
    _ping_srsp_cb,
    NULL,
    NULL,
    _reset_ind_cb,
    NULL,
    NULL,
    _osal_nv_write_srsp_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static void mt_sys_osal_nv_write(uint16_t id, uint8_t offset, uint8_t length, uint8_t *data)
{
    if(!data)
        return;

    OsalNvWriteFormat_t req;
    req.Id = id;
    req.Offset = offset;
    req.Len = length;
    memcpy(req.Value, data, length);
    sysOsalNvWrite(&req);
}

/********************************
 *          API                 *
 *******************************/

void mt_sys_register_callbacks(void)
{
    sysRegisterCallbacks(mt_sys_cb);
}

void mt_sys_reset_dongle (SyncActionCb cb)
{
    LOG_INF("Resetting ZNP");
    if(cb)
        sync_action_cb = cb;
    ResetReqFormat_t resReq;
    resReq.Type = 1;
    sysResetReq(&resReq);
}

void mt_sys_ping_dongle(SyncActionCb cb)
{
    LOG_INF("Ping dongle");
    if(cb)
        sync_action_cb = cb;
    sysPing();
}

void mt_sys_nv_write_clear_flag(SyncActionCb cb)
{
    LOG_INF("Writing NV clear flag");
    if(cb)
        sync_action_cb = cb;
    mt_sys_osal_nv_write(3, 0, 1, nv_clear_data);
}

void mt_sys_nv_write_coord_flag(SyncActionCb cb)
{
    LOG_INF("Setting device as coordinator");
    if(cb)
        sync_action_cb = cb;
    mt_sys_osal_nv_write(0x87, 0, 1, nv_coord_data);
}

void mt_sys_nv_write_disable_security(SyncActionCb cb)
{
    LOG_INF("Disabling NWK security");
    if(cb)
        sync_action_cb = cb;
    mt_sys_osal_nv_write(0x64, 0, 1, nv_disable_sec_data);
}

void mt_sys_nv_set_pan_id(SyncActionCb cb)
{
    LOG_INF("Setting PAN ID");
    if(cb)
        sync_action_cb = cb;
    mt_sys_osal_nv_write(0x83, 0, 2, nv_set_pan_id);
}

