#include <stdlib.h>
#include "mt_sys.h"
#include "dbgPrint.h"
#include "mtSys.h"

static uint8_t mt_sys_reset_ind_cb(ResetIndFormat_t *msg)
{
    log_inf("System reset ind. received. Reason : %s ",
            msg->Reason == 0 ? "power up":(msg->Reason == 1 ? "External":"Watchdog"));
    log_inf("Transport version : %d", msg->TransportRev);
    log_inf("Product ID : %d", msg->ProductId);
    log_inf("Major version : %d", msg->MajorRel);
    log_inf("Minor version : %d", msg->MinorRel);
    log_inf("Hardware version : %d", msg->HwRev);

    return 0;
}

static mtSysCb_t mt_sys_cb = {
    NULL,
    NULL,
    NULL,
    mt_sys_reset_ind_cb,
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
};

void mt_sys_register_callbacks(void)
{
    sysRegisterCallbacks(mt_sys_cb);
}

