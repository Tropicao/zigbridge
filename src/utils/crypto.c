#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "crypto.h"
#include "keys.h"
#include "utils.h"
#include "logs.h"

static int _init_count = 0;
static int _log_domain = -1;

int zg_crypto_init(void)
{
    ENSURE_SINGLE_INIT(_init_count);
    _log_domain = zg_logs_domain_register("zg_crypto", ZG_COLOR_LIGHTMAGENTA);
    INF("Initializing crypto module");
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    return 0;
}

void zg_crypto_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
}


uint8_t zg_crypto_encrypt_aes_ecb(uint8_t *data, uint8_t size, uint8_t *key, uint8_t *encrypted)
{
    EVP_CIPHER_CTX *ctx = NULL;
    int outlen;

    if(!data || size <= 0)
    {
        ERR("Cannot encrypt data : payload is empty or size is non positive");
        return -1;
    }

    if(!encrypted)
    {
        ERR("Cannot encrypt data : result variable is NULL");
        return -1;
    }

    if(!key)
    {
        ERR("Cannot encrypt data : provided key is empty");
        return -1;
    }
    ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL);
    EVP_EncryptUpdate(ctx, encrypted, &outlen, data, size);
    EVP_EncryptFinal_ex(ctx, encrypted + outlen, &outlen);
    DBG("Stored %d bytes of encrypted data", outlen);
    EVP_CIPHER_CTX_free(ctx);

    return 0;
}


