#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <uv.h>
#include <znp.h>
#include "keys.h"
#include "errno.h"

#define NETWORK_KEY_FILE        "/etc/zll-gateway/network.key"
#define NETWORK_KEY_SIZE        16

static uint8_t *nwk_key = NULL;

void _store_network_key()
{
    int fd = -1;

    if(!nwk_key)
    {
        LOG_ERR("Cannot store network key : key is empty");
        return;
    }

    fd = open(NETWORK_KEY_FILE, O_WRONLY|O_CREAT);
    if(fd < 0)
    {
        LOG_WARN("Network key file cannot be opened to store key");
        return;
    }

    if(write(fd, nwk_key, NETWORK_KEY_SIZE) < 0)
        LOG_ERR("Cannot store network key to file : %s", strerror(errno));
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
        LOG_ERR("Cannot open /dev/urandom");
        return NULL;
    }

    result = calloc(NETWORK_KEY_SIZE, sizeof(uint8_t));
    if(!result)
    {
        LOG_CRI("Cannot allocate memory for network key");
        uv_stop(uv_default_loop());
    }
    if(read(fd, result, NETWORK_KEY_SIZE) < 0)
    {
        LOG_ERR("Cannot generate new network key : %s", strerror(errno));
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

    fd = open(NETWORK_KEY_FILE, O_RDONLY);
    if(fd < 0)
    {
        LOG_WARN("Network key file cannot be opened, generating a new one");
        nwk_key = _generate_new_network_key();
        _store_network_key();
        return;
    }

    nwk_key = calloc(NETWORK_KEY_SIZE, sizeof(uint8_t));
    if(!nwk_key)
    {
        LOG_CRI("Cannot allocate memory for network key");
        uv_stop(uv_default_loop());
    }

    if(read(fd, nwk_key, NETWORK_KEY_SIZE) < 0)
    {
        LOG_ERR("Cannot read network key from file : %s", strerror(errno));
        free(nwk_key);
        nwk_key = NULL;
    }
    close(fd);
    return;
}

uint8_t *zg_keys_network_key_get()
{
    if(!nwk_key)
        _load_network_key();
    return nwk_key;
}

uint8_t zg_keys_network_key_size_get()
{
    return NETWORK_KEY_SIZE;
}



