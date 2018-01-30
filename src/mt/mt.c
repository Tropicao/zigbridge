#include <stdlib.h>
#include "mt.h"
#include "mt_af.h"
#include "mt_sys.h"
#include "mt_util.h"
#include "mt_zdo.h"
#include "conf.h"
#include "utils.h"

/********************************
 *         Local variables      *
 *******************************/
static int _init_count = 0;

/********************************
 *          API                 *
 *******************************/
int zg_mt_init()
{
    ENSURE_SINGLE_INIT(_init_count);

    int res = 0;
    res |= zg_mt_af_init();
    res |= zg_mt_sys_init();
    res |= zg_mt_util_init();
    res |= zg_mt_zdo_init();

    return res;
}

void zg_mt_shutdown()
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    zg_mt_zdo_shutdown();
    zg_mt_util_shutdown();
    zg_mt_sys_shutdown();
    zg_mt_af_shutdown();
}

void zg_mt_test_ping(void)
{
    zg_mt_sys_ping(NULL);
}
