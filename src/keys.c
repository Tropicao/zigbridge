#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <uv.h>
#include <errno.h>
#include "logs.h"
#include "keys.h"
#include "conf.h"
#include "utils.h"

#define NETWORK_KEY_SIZE        16

static uint8_t *nwk_key = NULL;
static int _log_domain = -1;

void _store_network_key()
{
    int fd = -1;

    if(!nwk_key)
    {
        ERR("Cannot store network key : key is empty");
        return;
    }

    fd = open(zg_conf_get_network_key_path(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    if(fd < 0)
    {
        WRN("Network key file cannot be opened to store key");
        return;
    }

    if(write(fd, nwk_key, NETWORK_KEY_SIZE) < 0)
        ERR("Cannot store network key to file : %s", strerror(errno));
    close(fd);
    return;
}


uint8_t *_generate_new_network_key()
{
    uint8_t *result = NULL;
    int fd = -1;

    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0)
    {
        ERR("Cannot open /dev/urandom");
        return NULL;
    }

    result = calloc(NETWORK_KEY_SIZE, sizeof(uint8_t));
    if(!result)
    {
        CRI("Cannot allocate memory for network key");
        uv_stop(uv_default_loop());
    }
    if(read(fd, result, NETWORK_KEY_SIZE) < 0)
    {
        ERR("Cannot generate new network key : %s", strerror(errno));
        free(nwk_key);
        nwk_key = NULL;
    }
    close(fd);

    return result;
}

void _load_network_key(void)
{
    int fd = -1;

    if(nwk_key)
        free(nwk_key);

    fd = open(zg_conf_get_network_key_path(), O_RDONLY);
    if(fd < 0)
    {
        WRN("Network key file cannot be opened, generating a new one");
        nwk_key = _generate_new_network_key();
        _store_network_key();
        return;
    }

    nwk_key = calloc(NETWORK_KEY_SIZE, sizeof(uint8_t));
    if(!nwk_key)
    {
        CRI("Cannot allocate memory for network key");
        uv_stop(uv_default_loop());
    }

    if(read(fd, nwk_key, NETWORK_KEY_SIZE) < 0)
    {
        ERR("Cannot read network key from file : %s", strerror(errno));
        free(nwk_key);
        nwk_key = NULL;
    }
    close(fd);
    return;
}

void zg_keys_init()
{
    _log_domain = zg_logs_domain_register("zg_keys", ZG_COLOR_LIGHTRED);
    if(!nwk_key)
        _load_network_key();
}

void zg_keys_shutdown()
{
    ZG_VAR_FREE(nwk_key);
}

uint8_t *zg_keys_network_key_get()
{
    return nwk_key;
}

uint8_t zg_keys_network_key_size_get()
{
    return NETWORK_KEY_SIZE;
}

void zg_keys_network_key_del(void)
{
    unlink(zg_conf_get_network_key_path());
}

uint8_t zg_keys_check_network_key_exists(void)
{
    return !access(zg_conf_get_network_key_path(), F_OK);
}
