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

#define KEY_SIZE            16

typedef enum
{
    KEY_NWK_KEY,
    KEY_ZLL_MASTER_KEY,
    KEY_ZHA_MASTER_KEY,
} KeyType;


static uint8_t *nwk_key = NULL;
static uint8_t *zll_master_key = NULL;
static uint8_t zha_master_key[] = {
    0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C,
    0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39
};

    ;
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

    if(write(fd, nwk_key, KEY_SIZE) < 0)
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

    result = calloc(KEY_SIZE, sizeof(uint8_t));
    if(!result)
    {
        CRI("Cannot allocate memory for network key");
        uv_stop(uv_default_loop());
    }
    if(read(fd, result, KEY_SIZE) < 0)
    {
        ERR("Cannot generate new network key : %s", strerror(errno));
        free(nwk_key);
        nwk_key = NULL;
    }
    close(fd);

    return result;
}

void _load_key(KeyType type)
{
    int fd = -1;
    uint8_t *key_table;
    const char *key_path;

    switch(type)
    {
        case KEY_NWK_KEY:
            key_table = nwk_key;
            key_path = zg_conf_get_network_key_path();
            break;
        case KEY_ZLL_MASTER_KEY:
            key_table = zll_master_key;
            key_path = zg_conf_get_zll_master_key_path();
            break;
        default:
            return;
            break;
    }

    if(key_table)
        free(key_table);

    fd = open(key_path, O_RDONLY);
    if(fd < 0)
    {
        if(type == KEY_NWK_KEY)
        {
            WRN("Key file cannot be opened, generating a new one");
            nwk_key = _generate_new_network_key();
            _store_network_key();
        }
        else
        {
            WRN("No key file found, abort key loading");
        }
            return;
    }

    key_table = calloc(KEY_SIZE, sizeof(uint8_t));
    if(!key_table)
    {
        CRI("Cannot allocate memory for key");
        uv_stop(uv_default_loop());
    }

    if(read(fd, key_table, KEY_SIZE) < 0)
    {
        ERR("Cannot read key from file %s : %s", key_path, strerror(errno));
        ZG_VAR_FREE(key_table);
    }
    close(fd);
    return;
}

void _load_nwk_key(void)
{
    _load_key(KEY_NWK_KEY);
}

void _load_zll_master_key(void)
{
    _load_key(KEY_ZLL_MASTER_KEY);
}

void zg_keys_init()
{
    _log_domain = zg_logs_domain_register("zg_keys", ZG_COLOR_LIGHTRED);
    if(!nwk_key)
        _load_nwk_key();
    if(!zll_master_key)
        _load_zll_master_key();
}

void zg_keys_shutdown()
{
    ZG_VAR_FREE(nwk_key);
    ZG_VAR_FREE(zll_master_key);
}

uint8_t *zg_keys_network_key_get()
{
    return nwk_key;
}

uint8_t *zg_keys_zll_master_key_get()
{
    return zll_master_key;
}

uint8_t *zg_keys_zha_master_key_get()
{
    return zha_master_key;
}

uint8_t zg_keys_size_get()
{
    return KEY_SIZE;
}

void zg_keys_network_key_del(void)
{
    unlink(zg_conf_get_network_key_path());
}

uint8_t zg_keys_check_network_key_exists(void)
{
    return !access(zg_conf_get_network_key_path(), F_OK);
}
