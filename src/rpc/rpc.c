#include <stdint.h>
#include "types.h"
#include "rpc.h"

uint8_t zg_rpc_init(char *device __attribute__((unused)))
{

    return 0;
}


void zg_rpc_shutdown(char *device __attribute__((unused)))
{

}

void zg_rpc_read(void)
{

}

uint8_t zg_rpc_write(uint8_t *data __attribute__((unused)), int len __attribute__((unused)))
{

    return 0;
}

int zg_rpc_get_fd(void)
{

    return -1;
}
