#include "app.h"
#include "mt_sys.h"
#include "dbgPrint.h"
#include "mtSys.h"
#include <rpc.h>

uv_async_t state_flag;

void reset_dongle (void)
{
    log_inf("Resetting ZNP");
    ResetReqFormat_t resReq;
    resReq.Type = 1;
    sysResetReq(&resReq);
    rpcWaitMqClientMsg(5000);
}

void app_register_callbacks()
{
    mt_sys_register_callbacks();
}

void state_machine_cb(uv_async_t *state_data __attribute__((unused)))
{
    log_inf("State machine has been woken up !!!");
}

