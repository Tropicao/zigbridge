#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "zha.h"
#include "aps.h"
#include "zcl.h"
#include "mt_af.h"
#include "mt_zdo.h"
#include "mt_sys.h"
#include "action_list.h"
#include "logs.h"
#include "utils.h"

/********************************
 *          Constants           *
 *******************************/

#define ZHA_ENDPOINT                            0x1

/********** Format **********/

#define COMMAND_OFF                             0x00
#define COMMAND_ON                              0x01
#define COMMAND_TOGGLE                          0x02
#define COMMAND_REPORT_ATTRIBUTE                0x0A
#define COMMAND_OFF_WITH_EFFECT                 0x40
#define COMMAND_ON_WITH_RECALL_GLOBAL_SCENE     0x41

/* Off with effect frame format */
#define LEN_OFF_WITH_EFFECT                     2
#define INDEX_EFFECT_IDENTIFIER                 0
#define INDEX_EFFeCT_VARIANT                    0

/* Data */

/* Scan request */
#define DEVICE_TYPE_COORDINATOR                 0x0
#define DEVICE_TYPE_ROUTER                      0x1
#define DEVICE_TYPE_END_DEVICE                  0x2

#define RX_ON_WHEN_IDLE                         (0x1<<2)
#define RX_OFF_WHEN_IDLE                        (0x0<<2)

/* Identify Request */
#define EXIT_IDENTIFY                           0x0000
#define DEFAULT_IDENTIFY_DURATION               0xFFFF

/********** Application data **********/

/* Off with effect */
#define DELAYED_ALL_OFF                         0x00
#define DYING_LIGHT                             0x01

/* App data */
#define ZHA_PROFIL_ID                           0x0104  /* ZLL */
#define ZHA_DEVICE_ID                           0X0210  /* Extended color light */
#define ZHA_DEVICE_VERSION                      0x2     /* Version 2 */

/********************************
 *          Local variables     *
 *******************************/

static int _init_count = 0;
static int _log_domain = -1;

static void (*_button_change_cb)(uint16_t short_addr, uint8_t state) = NULL;
static void (*_temperature_cb)(uint16_t short_addr, int16_t temp) = NULL;
static void (*_humidity_cb)(uint16_t short_addr, uint16_t humidity) = NULL;
static void (*_pressure_cb)(uint16_t short_addr, int16_t pressure) = NULL;
static NewDeviceJoinedCb _new_device_ind_cb = NULL;

static uint16_t _zha_in_clusters[] = {
    ZCL_CLUSTER_ON_OFF};
static uint8_t _zha_in_clusters_num = sizeof(_zha_in_clusters)/sizeof(uint8_t);

static uint16_t _zha_out_clusters[] = {
    ZCL_CLUSTER_ON_OFF};
static uint8_t _zha_out_clusters_num = sizeof(_zha_out_clusters)/sizeof(uint8_t);

/********************************
 *   ZHA messages callbacks     *
 *******************************/
static void _process_on_off_command(uint16_t addr, void *data, int len __attribute__((unused)))
{
    uint8_t *buffer = (uint8_t *) data;
    INF("Processing command on On/Off cluster");

    if(buffer[2] == COMMAND_REPORT_ATTRIBUTE)
    {
        INF("Received new switch status : %s", buffer[6] ? "Released":"Pushed");
        if(_button_change_cb && buffer[6] == 0x00)
            _button_change_cb(addr, buffer[6]);
    }
    else
    {
        WRN("Unuspported On/Off command/status 0x%02X", buffer[2]);
    }
}

static void _process_temperature_measurement_command(uint16_t addr, void *data, int len __attribute__((unused)))
{
    uint8_t *buffer = (uint8_t *)data;
    int16_t temp;
    if(buffer[2] == COMMAND_REPORT_ATTRIBUTE)
    {
        INF("Received new temperature status");
        if(_temperature_cb)
        {
            memcpy(&temp, buffer+6, 2);
            _temperature_cb(addr, temp);
        }
    }
    else
    {
        WRN("Unuspported temperature measurement command/status 0x%02X", buffer[2]);
    }
}

static void _process_pressure_measurement_command(uint16_t addr, void *data, int len __attribute__((unused)))
{
    uint8_t *buffer = (uint8_t *)data;
    int16_t pressure;
    if(buffer[2] == COMMAND_REPORT_ATTRIBUTE)
    {
        INF("Received new pressure status");
        if(_pressure_cb)
        {
            memcpy(&pressure, buffer+6, 2);
            _pressure_cb(addr, pressure);
        }
    }
    else
    {
        WRN("Unuspported pressure measurement command/status 0x%02X", buffer[2]);
    }
}

static void _process_humidity_measurement_command(uint16_t addr, void *data, int len __attribute__((unused)))
{
    uint8_t *buffer = (uint8_t *)data;
    uint16_t humidity;
    if(buffer[2] == COMMAND_REPORT_ATTRIBUTE)
    {
        INF("Received new humidity status");
        if(_humidity_cb)
        {
            memcpy(&humidity, buffer+6, 2);
            _humidity_cb(addr, humidity);
        }
    }
    else
    {
        WRN("Unuspported pressure measurement command/status 0x%02X", buffer[2]);
    }
}


static void _zha_message_cb(uint16_t addr, uint16_t cluster, void *data, int len)
{
    uint8_t *buffer = data;
    if(!buffer || len <= 0)
        return;

    DBG("Received ZHA data (%d bytes)", len);
    switch(cluster)
    {
        case ZCL_CLUSTER_ON_OFF:
            _process_on_off_command(addr, data, len);
            break;
        case ZCL_CLUSTER_TEMPERATURE_MEASUREMENT:
            _process_temperature_measurement_command(addr, data, len);
            break;
        case ZCL_CLUSTER_PRESSURE_MEASUREMENT:
            _process_pressure_measurement_command(addr, data, len);
            break;
        case ZCL_CLUSTER_HUMIDITY_MEASUREMENT:
            _process_humidity_measurement_command(addr, data, len);
            break;
        default:
            WRN("Unsupported ZHA cluster 0x%04X", cluster);
            break;
    }
}

static void _zha_visible_device_cb(uint16_t addr, uint64_t ext_addr)
{
    INF("New device joined network with address 0x%04X !", addr);
    if(_new_device_ind_cb)
        _new_device_ind_cb(addr, ext_addr);

}

/********************************
 * Initialization state machine *
 *******************************/

static ZgAl *_init_sm = NULL;

/* Callback triggered when ZHA initialization is complete */
static InitCompleteCb _init_complete_cb = NULL;

static void _general_init_cb(void)
{
    if(zg_al_continue(_init_sm) != 0)
    {
        INF("ZHA application is initialized");
        zg_al_destroy(_init_sm);
        _init_sm = NULL;
        if(_init_complete_cb)
            _init_complete_cb();
    }
}

void _register_zha_endpoint(SyncActionCb cb)
{
    zg_aps_register_endpoint(   ZHA_ENDPOINT,
                                ZHA_PROFIL_ID,
                                ZHA_DEVICE_ID,
                                ZHA_DEVICE_VERSION,
                                _zha_in_clusters_num,
                                _zha_in_clusters,
                                _zha_out_clusters_num,
                                _zha_out_clusters,
                                _zha_message_cb,
                                cb);
}


static ZgAlState _init_states[] = {
    {_register_zha_endpoint, _general_init_cb}
};
static int _init_nb_states = sizeof(_init_states)/sizeof(ZgAlState);

/********************************
 *          ZLL API             *
 *******************************/

uint8_t zg_zha_init(InitCompleteCb cb)
{
    ENSURE_SINGLE_INIT(_init_count);
    zg_aps_init();
    _log_domain = zg_logs_domain_register("zg_zha", ZG_COLOR_LIGHTCYAN);
    INF("Initializing ZHA");
    if(cb)
        _init_complete_cb = cb;

    zg_mt_zdo_register_visible_device_cb(_zha_visible_device_cb);
    _init_sm = zg_al_create(_init_states, _init_nb_states);
    zg_al_continue(_init_sm);
    return 0;
}

void zg_zha_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    zg_aps_shutdown();
    zg_al_destroy(_init_sm);
}

void zg_zha_on_off_set(uint16_t addr, uint8_t endpoint, uint8_t state)
{
    uint8_t command = state ? COMMAND_ON:COMMAND_OFF;

    zg_aps_send_data(addr,
            0xABCD,
            ZHA_ENDPOINT,
            endpoint,
            ZCL_CLUSTER_ON_OFF,
            command,
            NULL,
            0,
            NULL);
}

void zg_zha_register_device_ind_callback(NewDeviceJoinedCb cb)
{
    _new_device_ind_cb = cb;
}

void zg_zha_register_button_state_cb(void (*cb)(uint16_t short_addr, uint8_t state))
{
    _button_change_cb = cb;
}

void zg_zha_register_temperature_cb(void (*cb)(uint16_t short_addr, int16_t temp))
{
    _temperature_cb = cb;
}

void zg_zha_register_humidity_cb(void (*cb)(uint16_t short_addr, uint16_t humidity))
{
    _humidity_cb = cb;
}

void zg_zha_register_pressure_cb(void (*cb)(uint16_t short_addr, int16_t pressure))
{
    _pressure_cb = cb;
}

void zg_zha_move_to_color(uint16_t short_addr, uint16_t x, uint16_t y, uint8_t duration_s)
{
    uint8_t command[6];
    uint16_t duration = duration_s * 10;

    memcpy(command, &x, 2);
    memcpy(command+2, &y, 2);
    memcpy(command+4, &duration, 2);

    zg_aps_send_data(short_addr,
            0xABCD,
            ZHA_ENDPOINT,
            0x0B,
            ZCL_CLUSTER_COLOR_CONTROL,
            0x07,
            command,
            6,
            NULL);
}


