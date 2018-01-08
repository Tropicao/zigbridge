#include <string.h>
#include <iniparser.h>
#include <znp.h>
#include "conf.h"
#include "utils.h"

#define SECTION_KEY_STRING_MAX_SIZE 128
#define STRING_VALUE_MAX_SIZE       128
#define DEFAULT_CONFIG_PATH         "/etc/zll-gateway/config.ini"

#define SECTION_SECURITY            "security"
#define KEY_NETWORK_KEY_PATH            "network_key_path"
#define SECTION_DEVICES             "devices"
#define KEY_DEVICE_LIST_PATH            "device_list_path"

#define PRINT_STRING_VALUE(section, key, val)   {LOG_INF("%s/%s : %s", section, key, val?val:"NULL");}

/**
 * \brief This module holds all configuration management.
 *
 * Adding a new configuration field require the following steps :
 * * Adding a field in the Configuration structure
 * * Define Section and key used to retrieve the value, use it in zg_conf_load
 * with _load_value
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
} Configuration;

typedef enum
{
    CONF_VAL_INT,
    CONF_VAL_REAL,
    CONF_VAL_STRING
} ValueType;

static Configuration _configuration;


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
        LOG_WARN("Cannot build iniparser key (section %s, key %s)", section, key);
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
                    LOG_CRI("Cannot allocate memory for string value configuration");
            }
            break;
        default:
            LOG_WARN("Unknown value type asked");
            break;
    }
}

static void _print_configuration()
{
    PRINT_STRING_VALUE(SECTION_SECURITY, KEY_NETWORK_KEY_PATH, _configuration.network_key_path);
    PRINT_STRING_VALUE(SECTION_DEVICES, KEY_DEVICE_LIST_PATH, _configuration.device_list_path);
}
/****************************************
 *                  API                 *
 ***************************************/

int zg_conf_load(char *conf_path)
{

    dictionary *dict;
    char *path = (conf_path ? conf_path:DEFAULT_CONFIG_PATH);

    memset(&_configuration, 0, sizeof(_configuration));
    dict = iniparser_load(path);
    if(!dict)
    {
        LOG_CRI("Cannot open configuration file %s", path);
        return 1;
    }
    else
    {
        _load_value(dict, SECTION_SECURITY, KEY_NETWORK_KEY_PATH, &(_configuration.network_key_path), CONF_VAL_STRING);
        _load_value(dict, SECTION_DEVICES, KEY_DEVICE_LIST_PATH, &(_configuration.device_list_path), CONF_VAL_STRING);
        iniparser_freedict(dict);
    }
    _print_configuration();
    return 0;
}

void zg_conf_free()
{
    ZG_VAR_FREE(_configuration.network_key_path);
    ZG_VAR_FREE(_configuration.device_list_path);
    memset(&_configuration, 0, sizeof(_configuration));
}


const char *zg_conf_get_network_key_path()
{
    return _configuration.network_key_path;
}

const char *zg_conf_get_device_list_path()
{
    return _configuration.device_list_path;
}

