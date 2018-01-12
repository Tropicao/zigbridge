#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uv.h>
#include <znp.h>
#include "core.h"
#include "conf.h"
#include "types.h"
#include "ipc.h"

uv_loop_t *loop = NULL;
uv_poll_t znp_poll;
uv_poll_t user_poll;
static uint8_t _reset_network = 0;

static void signal_handler(uv_signal_t *handle __attribute__((unused)), int signum __attribute__((unused)))
{
    LOG_INF("Application received SIGINT");
    uv_stop(loop);
}

static void znp_poll_cb(uv_poll_t *handle __attribute__((unused)), int status, int events)
{
    if(status < 0)
    {
        LOG_ERR("ZNP socket error %s (%s)", uv_err_name(status), uv_strerror(status));
        return;
    }

    if(events & UV_READABLE)
    {
        LOG_DBG("ZNP socket has received data");
        znp_loop_read();
    }
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    uv_signal_t sig_int;
    int znp_fd = -1, user_fd =0;
    int status = -1;
    char config_file_path[PATH_STRING_MAX_SIZE] = {0};
    int c = 0;

    while ((c = getopt (argc, argv, "c:r")) != -1)
    {
        switch (c)
        {
            case 'c':
                strncpy(config_file_path,optarg, PATH_STRING_MAX_SIZE);
                break;
            case 'r':
                _reset_network = 1;
                break;
            case '?':
                if (optopt == 'c')
                    LOG_ERR("Option -%c requires an argument", optopt);
                else
                    LOG_ERR("Unknown option character `\\x%x'", optopt);
                return 1;
                break;
            default:
                break;
        }
    }

    srand(time(NULL));
    if(config_file_path[0] != 0)
        status = zg_conf_load(config_file_path);
    else
        status = zg_conf_load(NULL);

    if(status != 0)
        return 1;

    loop = uv_default_loop();
    if(znp_init() != 0)
    {
        LOG_CRI("Cannot initialize ZNP library");
        return 1;
    }
    znp_fd = znp_socket_get();
    if(znp_fd < 0)
    {
        LOG_CRI("Cannot get ZNP socket descriptor");
        return 1;
    }
    status = uv_poll_init(loop, &znp_poll, znp_fd);
    if(status < 0)
    {
        LOG_ERR("Cannot add ZNP socket descriptor (%d) to main event loop : %s (%s)",
                znp_fd, uv_err_name(status), uv_strerror(status));
        return 1;
    }
    status = uv_poll_init(loop, &user_poll, user_fd);

    uv_poll_start(&znp_poll, UV_READABLE, znp_poll_cb);
    uv_poll_start(&user_poll, UV_READABLE, zg_ipc_get_ipc_main_callback());

    uv_signal_init(loop, &sig_int);

    uv_signal_start(&sig_int, signal_handler, SIGINT);

    zg_core_init(_reset_network);
    LOG_INF("Starting main loop");
    uv_run(loop, UV_RUN_DEFAULT);


    LOG_INF("Quitting application");
    zg_core_shutdown();
    uv_poll_stop(&znp_poll);
    znp_shutdown();
    zg_conf_free();
    exit(0);
}
