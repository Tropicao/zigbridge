#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uv.h>
#include <Eina.h>
#include "core.h"
#include "conf.h"
#include "types.h"
#include "mt.h"
#include "rpc.h"
#include "logs.h"

uv_loop_t *loop = NULL;
uv_poll_t znp_poll;
uv_poll_t user_poll;
static uint8_t _reset_network = 0;
static int _log_domain = -1;

static void signal_handler(uv_signal_t *handle __attribute__((unused)), int signum __attribute__((unused)))
{
    INF("Application received SIGINT");
    uv_stop(loop);
}

static void znp_poll_cb(uv_poll_t *handle __attribute__((unused)), int status, int events)
{
    if(status < 0)
    {
        ERR("ZNP socket error : %s (%s)", uv_err_name(status), uv_strerror(status));
        return;
    }

    if(events & UV_READABLE)
    {
        DBG("ZNP socket has received data");
        zg_rpc_read();
    }
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    uv_signal_t sig_int;
    int znp_fd = -1, user_fd =0;
    int status = -1;
    char config_file_path[PATH_STRING_MAX_SIZE] = {0};
    int c = 0;

    zg_logs_init();

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
                    ERR("Option -%c requires an argument", optopt);
                else
                    ERR("Unknown option character `\\x%x'", optopt);
                return 1;
                break;
            default:
                break;
        }
    }

    _log_domain = zg_logs_domain_register("zg_main", ZG_COLOR_BLACK);
    srand(time(NULL));
    if(config_file_path[0] != 0)
        status = zg_conf_init(config_file_path);
    else
        status = zg_conf_init(NULL);

    if(status != 0)
    {
        ERR("Cannot load configuration, abort");
        goto main_end;
    }

    loop = uv_default_loop();

    if(zg_mt_init() != 0)
    {
        CRI("Cannot initialize ZNP medium");
        goto main_end;
    }
    znp_fd = zg_rpc_get_fd();

    status = uv_poll_init(loop, &znp_poll, znp_fd);
    if(status < 0)
    {
        ERR("Cannot add ZNP socket descriptor (%d) to main event loop : %s (%s)",
                znp_fd, uv_err_name(status), uv_strerror(status));
        goto main_end;
    }

    status = uv_poll_init(loop, &user_poll, user_fd);
    uv_poll_start(&znp_poll, UV_READABLE, znp_poll_cb);

    uv_signal_init(loop, &sig_int);
    uv_signal_start(&sig_int, signal_handler, SIGINT);

    zg_mt_test_ping();
    INF("Starting main loop");
    uv_run(loop, UV_RUN_DEFAULT);


main_end:
    INF("Quitting application");
    uv_signal_stop(&sig_int);
    uv_poll_stop(&user_poll);
    uv_poll_stop(&znp_poll);
    zg_mt_shutdown();
    zg_conf_shutdown();
    zg_logs_shutdown();
    exit(0);
}
