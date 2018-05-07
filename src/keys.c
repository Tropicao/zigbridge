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
#include "crypto.h"

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

static int _log_domain = -1;

static void _dump_key(const char *name, uint8_t *key)
{
    DUMP_DATA(name, key, KEY_SIZE);
}

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

uint8_t *_load_key(KeyType type)
{
    int fd = -1;
    uint8_t *key_table = NULL;
    const char *key_path;

    switch(type)
    {
        case KEY_NWK_KEY:
            key_path = zg_conf_get_network_key_path();
            break;
        case KEY_ZLL_MASTER_KEY:
            key_path = zg_conf_get_zll_master_key_path();
            break;
        default:
            return NULL;
            break;
    }

    fd = open(key_path, O_RDONLY);
    if(fd < 0)
    {
        if(type == KEY_NWK_KEY)
        {
            WRN("Key file cannot be opened, generating a new one");
            nwk_key = _generate_new_network_key();
            _store_network_key();
            return nwk_key;
        }
        else
        {
            WRN("No key file found, abort key loading");
        }
        return NULL;
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
    return key_table;
}

void _load_nwk_key(void)
{
    nwk_key = _load_key(KEY_NWK_KEY);
    DBG("Network key loaded");
}

void _load_zll_master_key(void)
{
    zll_master_key = _load_key(KEY_ZLL_MASTER_KEY);
    DBG("ZLL master key loaded");
}

uint8_t _encrypt_key(uint8_t *payload, uint8_t *key, uint8_t *encrypted)
{
    return zg_crypto_encrypt_aes_ecb(payload, KEY_SIZE, key, encrypted);
}


void zg_keys_init()
{
    _log_domain = zg_logs_domain_register("zg_keys", ZG_COLOR_LIGHTRED);
    zg_crypto_init();
    if(!nwk_key)
        _load_nwk_key();
    if(!zll_master_key)
        _load_zll_master_key();
}

void zg_keys_shutdown()
{
    zg_crypto_init();
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

uint8_t zg_keys_get_encrypted_network_key_for_zll(uint32_t transaction_id, uint32_t response_id, uint8_t *encrypted)
{
    uint8_t transport_key_plain[16] = {0};
    uint8_t transport_key_encrypted[16] = {0};
    uint8_t *nwk_key_plain = zg_keys_network_key_get();
    uint8_t nwk_r[16];
    uint8_t *master_key = zg_keys_zll_master_key_get();
    uint8_t zll[16];
    uint8_t transport_r [4] = {0};
    uint8_t response_r [4] = {0};
    uint8_t index = 0;

    if(!encrypted)
    {
        ERR("Cannot compute encrypted network key : encrypted variable is not allocated");
        return -1;
    }

    memcpy(transport_r, &transaction_id, 4);
    memcpy(response_r, &response_id, 4);
    memcpy(nwk_r, nwk_key_plain, 16);
    memcpy(zll, master_key, 16);
    for(index = 0; index < 4; index++)
        transport_r[3-index] = (transaction_id >> (index * 8)) & 0xFF;
    for(index = 0; index < 4; index++)
        response_r[3-index] = (response_id >> (index * 8)) & 0xFF;

    index = 0;
    memcpy(transport_key_plain + index, transport_r, sizeof(transaction_id));
    index += sizeof(transaction_id);
    memcpy(transport_key_plain + index, transport_r, sizeof(transaction_id));
    index += sizeof(transaction_id);
    memcpy(transport_key_plain + index, response_r, sizeof(response_id));
    index += sizeof(response_id);
    memcpy(transport_key_plain + index, response_r, sizeof(response_id));

    DBG("Transport Key : ");
    for(index = 0; index < 16; index++)
        DBG("[%d] 0x%02X", index, transport_key_plain[index]);

    if(_encrypt_key(transport_key_plain, zll, transport_key_encrypted) != 0
            || _encrypt_key(nwk_r, transport_key_encrypted, encrypted) !=0)
    {
        ERR("Network key encryption failed");
        return 1;
    }

    return 0;

}

uint8_t zg_keys_test_nwk_key_encryption_zll()
{
    uint8_t transport_key_plain[16] = {0};
    uint8_t transport_key_encrypted[16] = {0};
    uint8_t index = 0;
    uint8_t encrypted[16] = {0};

    /* Test data : see ZLL specification for dataset */
    uint32_t interpan_transaction_identifier = 0x0920aa3e;
    uint32_t touchlink_response_identifier = 0xb12f7688;
    uint8_t nwk_key_plain[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};

    memcpy(transport_key_plain + index, &interpan_transaction_identifier, sizeof(interpan_transaction_identifier));
    index += sizeof(interpan_transaction_identifier);
    memcpy(transport_key_plain + index, &interpan_transaction_identifier, sizeof(interpan_transaction_identifier));
    index += sizeof(interpan_transaction_identifier);
    memcpy(transport_key_plain + index, &touchlink_response_identifier, sizeof(touchlink_response_identifier));
    index += sizeof(touchlink_response_identifier);
    memcpy(transport_key_plain + index, &touchlink_response_identifier, sizeof(touchlink_response_identifier));

    if(_encrypt_key(transport_key_plain, zg_keys_zll_master_key_get(), transport_key_encrypted) != 0
            || _encrypt_key(nwk_key_plain, transport_key_encrypted, encrypted) !=0)
    {
        ERR("Test encryption failed");
        return 1;
    }

    _dump_key("Encrypted test result", encrypted);


    return 0;
}








