#include <string.h>
#include <iniparser/iniparser.h>
#include <stdlib.h>
#include "conf.h"
#include "utils.h"
#include "logs.h"

#define SECTION_KEY_STRING_MAX_SIZE 128
#define STRING_VALUE_MAX_SIZE       128
#define DEFAULT_CONFIG_PATH         "/etc/zigbdrige/config.ini"

#define SECTION_GENERAL             "general"
#define KEY_ZNP_DEVICE_PATH             "znp_device_path"
#define KEY_ZNP_BAUDRATE                "znp_baudrate"
#define SECTION_SECURITY            "security"
#define KEY_NETWORK_KEY_PATH            "network_key_path"
#define SECTION_DEVICES             "devices"
#define KEY_DEVICE_LIST_PATH            "device_list_path"
#define SECTION_HTTP_SERVER         "http_server"
#define KEY_HTTP_SERVER_ADDR            "http_server_address"
#define KEY_HTTP_SERVER_PORT            "http_server_port"
#define SECTION_TCP_SERVER         "tcp_server"
#define KEY_TCP_SERVER_ADDR            "tcp_server_address"
#define KEY_TCP_SERVER_PORT            "tcp_server_port"

#define PRINT_STRING_VALUE(section, key, val)   {INF("%s/%s : %s", section, key, val?val:"NULL");}
#define PRINT_INT_VALUE(section, key, val)      {INF("%s/%s : %d", section, key, val);}

/**
 * \brief This module holds all configuration management.
 *
 * Adding a new configuration field require the following steps :
 * * Adding a field in the Configuration structure
 * * Define Section and key used to retrieve the value, use it in zg_conf_init
 *   with _load_value
 * * Add a getter API to allow other modules to retrieve the new configuration
 * * Add print of retrieve value in the print function
 * * If new field is a string, add proper free in _free_configuration
 */


/**
 * Main structure which will hold all the configuration to be loaded.
 */
typedef struct
{
    char *network_key_path;
    char *device_list_path;
    char *znp_device_path;
    int znp_baudrate;
    char *http_server_address;
    int http_server_port;
    char *tcp_server_address;
    int tcp_server_port;
} Configuration;

typedef enum
{
    CONF_VAL_INT,
    CONF_VAL_REAL,
    CONF_VAL_STRING
} ValueType;

static Configuration _configuration;
static int _log_domain = -1;


/****************************************
 *          Internal functions          *
 ***************************************/

static void _load_value(dictionary *ini, char *section, char *key, void *var, ValueType type)
{
    char elem[SECTION_KEY_STRING_MAX_SIZE] = {0};
    int *int_value = NULL;
    double *real_value = NULL;

    /** String management */
    const char *string_value = NULL;
    char **string_p;
    int val_len = 0;

    if(!ini || !section || !key)
        return;

    if(snprintf(elem, SECTION_KEY_STRING_MAX_SIZE, "%s:%s", section, key) < 0)
    {
        WRN("Cannot build iniparser key (section %s, key %s)", section, key);
        return;
    }

    switch(type)
    {
        case CONF_VAL_INT:
            int_value = (int *)var;
            *int_value = iniparser_getint(ini, elem, 0);
            break;
        case CONF_VAL_REAL:
            real_value = (double *)var;
            *real_value = iniparser_getdouble(ini, elem, 0.0);
            break;
        case CONF_VAL_STRING:
            string_value = iniparser_getstring(ini, elem, NULL);
            if(string_value)
            {
                val_len = strlen(string_value);
                string_p = (char **)var;
                *string_p = calloc(val_len+1, sizeof(char));
                if(*string_p)
                    strncpy(*string_p, string_value, val_len + 1);
                else
                    CRI("Cannot allocate memory for string value configuration");
            }
            break;
        default:
            WRN("Unknown value type asked");
            break;
    }
}

static void _print_configuration()
{
    PRINT_STRING_VALUE(SECTION_GENERAL, KEY_ZNP_DEVICE_PATH, _configuration.znp_device_path);
    PRINT_INT_VALUE(SECTION_GENERAL, KEY_ZNP_BAUDRATE, _configuration.znp_baudrate);
    PRINT_STRING_VALUE(SECTION_SECURITY, KEY_NETWORK_KEY_PATH, _configuration.network_key_path);
    PRINT_STRING_VALUE(SECTION_DEVICES, KEY_DEVICE_LIST_PATH, _configuration.device_list_path);
    PRINT_STRING_VALUE(SECTION_HTTP_SERVER, KEY_HTTP_SERVER_ADDR, _configuration.http_server_address);
    PRINT_INT_VALUE(SECTION_HTTP_SERVER, KEY_HTTP_SERVER_PORT, _configuration.http_server_port);
    PRINT_STRING_VALUE(SECTION_TCP_SERVER, KEY_TCP_SERVER_ADDR, _configuration.tcp_server_address);
    PRINT_INT_VALUE(SECTION_TCP_SERVER, KEY_TCP_SERVER_PORT, _configuration.tcp_server_port);
}
/****************************************
 *                  API                 *
 ***************************************/

uint8_t zg_conf_init(char *conf_path)
{
    _log_domain = zg_logs_domain_register("zg_conf", ZG_COLOR_LIGHTBLUE);
    dictionary *dict;
    char *path = (conf_path ? conf_path:DEFAULT_CONFIG_PATH);

    memset(&_configuration, 0, sizeof(_configuration));
    dict = iniparser_load(path);
    if(!dict)
    {
        CRI("Cannot open configuration file %s", path);
        return 1;
    }
    else
    {
        _load_value(dict, SECTION_SECURITY, KEY_NETWORK_KEY_PATH, &(_configuration.network_key_path), CONF_VAL_STRING);
        _load_value(dict, SECTION_DEVICES, KEY_DEVICE_LIST_PATH, &(_configuration.device_list_path), CONF_VAL_STRING);
        _load_value(dict, SECTION_GENERAL, KEY_ZNP_DEVICE_PATH, &(_configuration.znp_device_path), CONF_VAL_STRING);
        _load_value(dict, SECTION_GENERAL, KEY_ZNP_BAUDRATE, &(_configuration.znp_baudrate), CONF_VAL_INT);
        _load_value(dict, SECTION_HTTP_SERVER, KEY_HTTP_SERVER_ADDR, &(_configuration.http_server_address), CONF_VAL_STRING);
        _load_value(dict, SECTION_HTTP_SERVER, KEY_HTTP_SERVER_PORT, &(_configuration.http_server_port), CONF_VAL_INT);
        _load_value(dict, SECTION_TCP_SERVER, KEY_TCP_SERVER_ADDR, &(_configuration.tcp_server_address), CONF_VAL_STRING);
        _load_value(dict, SECTION_TCP_SERVER, KEY_TCP_SERVER_PORT, &(_configuration.tcp_server_port), CONF_VAL_INT);
        iniparser_freedict(dict);
    }
    _print_configuration();
    return 0;
}

void zg_conf_shutdown()
{
    ZG_VAR_FREE(_configuration.network_key_path);
    ZG_VAR_FREE(_configuration.device_list_path);
    ZG_VAR_FREE(_configuration.http_server_address);
    ZG_VAR_FREE(_configuration.tcp_server_address);
    memset(&_configuration, 0, sizeof(_configuration));
}

const char *zg_conf_get_znp_device_path()
{
    return _configuration.znp_device_path;
}

int zg_conf_get_znp_baudrate()
{
    return _configuration.znp_baudrate;
}

const char *zg_conf_get_network_key_path()
{
    return _configuration.network_key_path;
}

const char *zg_conf_get_device_list_path()
{
    return _configuration.device_list_path;
}

const char *zg_conf_get_http_server_address()
{
    return _configuration.http_server_address;
}

int zg_conf_get_http_server_port()
{
    return _configuration.http_server_port;
}

const char *zg_conf_get_tcp_server_address()
{
    return _configuration.tcp_server_address;
}

int zg_conf_get_tcp_server_port()
{
    return _configuration.tcp_server_port;
}
