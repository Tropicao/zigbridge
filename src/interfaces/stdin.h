#ifndef ZG_STDIN_H
#define ZG_STDIN_H

#include <uv.h>

typedef enum
{
    ZG_STDIN_COMMAND_TOUCHLINK,
    ZG_STDIN_COMMAND_SWITCH_DEMO_LIGHT,
    ZG_STDIN_COMMAND_MOVE_TO_BLUE,
    ZG_STDIN_COMMAND_MOVE_TO_RED,
    ZG_STDIN_COMMAND_DISCOVERY,
    ZG_STDIN_COMMAND_OPEN_NETWORK,
    ZG_STDIN_COMMAND_NONE,
} StdinCommand;

typedef void (*stdin_fd_cb)(uv_poll_t *handler, int status, int events);
typedef void (*stdin_command_cb)(StdinCommand command);

int zg_stdin_init(void);
void zg_stdin_shutdown(void);
stdin_fd_cb zg_stdin_get_stdin_main_callback();
void zg_stdin_register_command_cb(stdin_command_cb cb);


#endif

