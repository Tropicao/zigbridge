#include <stdlib.h>
#include "mt.h"
#include "mt_sys.h"
#include "rpc.h"
#include "conf.h"

/********************************
 *          API                 *
 *******************************/
int zg_mt_init()
{
    int res = 0;
    res |= zg_rpc_init(zg_conf_get_znp_device_path());
    res |= zg_mt_sys_init();

    return res;
}

void zg_mt_shutdown()
{
    zg_mt_sys_shutdown();
    zg_rpc_shutdown();
}

void zg_mt_test_ping(void)
{
    zg_rpc_write(ZG_MT_CMD_SREQ, ZG_MT_SUBSYS_SYS, 0x01, NULL, 0);
}
