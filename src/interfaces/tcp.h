
#ifndef ZG_TCP_H
#define ZG_TCP_H

#include <uv.h>
#include "device.h"

typedef enum
{
    ZG_TCP_COMMAND_NONE,
    ZG_TCP_COMMAND_GET_DEVICE_LIST,
    ZG_TCP_COMMAND_TOUCHLINK,
} TcpCommand;

typedef enum
{
    ZG_TCP_EVENT_NEW_DEVICE = 0,
    ZG_TCP_EVENT_BUTTON_STATE,
    ZG_TCP_EVENT_TEMPERATURE,
    ZG_TCP_EVENT_PRESSURE,
    ZG_TCP_EVENT_HUMIDITY,
    ZG_TCP_EVENT_TOUCHLINK_FINISHED,
    ZG_TCP_EVENT_GUARD, /* Keep this value last */
} ZgTcpEvent;


typedef void (*tcp_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*tcp_command_cb)(TcpCommand command);

int zg_tcp_init();
void zg_tcp_shutdown();
void zg_tcp_register_command_cb(tcp_command_cb cb);
void zg_tcp_send_event(ZgTcpEvent event, json_t *data);


#endif

