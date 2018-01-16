#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <znp.h>
#include "mt_util.h"
#include "rpc.h"

/* Callback set for any synchronous operation (see SREQ in MT specification) */
static SyncActionCb sync_action_cb = NULL;

/********************************
 *       Constant data          *
 *******************************/


/********************************
 *     MT UTIL callbacks         *
 *******************************/

static uint8_t _callback_sub_cmd_srsp_cb(CallbackSubCmdSrspFormat_t *msg)
{
    if(msg->Status != ZSuccess)
        LOG_ERR("Cannot subscribe to callback : %s", znp_strerror(msg->Status));
    else
        LOG_INF("Callback subscription validated");
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static mtUtilCb_t mt_util_cb = {
    _callback_sub_cmd_srsp_cb
};

/********************************
 *          API                 *
 *******************************/

void mt_util_register_callbacks(void)
{
    utilRegisterCallbacks(mt_util_cb);
}

void mt_util_af_subscribe_cmd (SyncActionCb cb)
{
    LOG_INF("Subscribing to MT_AF callbacks");
    sync_action_cb = cb;
    CallbackSubCmdFormat_t req;
    req.SubsystemId = MT_AF_CB;
    req.Action = MT_CB_ACTION_ENABLE;
    utilCallbackSubCmd(&req);
}
