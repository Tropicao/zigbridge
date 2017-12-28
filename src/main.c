#include <stdio.h>
#include <stdlib.h>
#include <dbgPrint.h>
#include <uv.h>
#include <znp.h>
#include "zll.h"
#include <unistd.h>

#define SERIAL_DEVICE   "/dev/ttyACM0"

uv_loop_t *loop = NULL;
uv_poll_t znp_poll;

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

static void _init_complete_cb(void)
{
    zg_zll_start_touchlink();
}


int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    uv_signal_t sig_int;
    int znp_fd = -1;
    int status = -1;

    loop = uv_default_loop();
    if(znp_init() != 0)
    {
        LOG_CRI("Cannot initialize ZNP library");
        free(loop);
        return 1;
    }
    znp_fd = znp_socket_get();
    if(znp_fd < 0)
    {
        LOG_CRI("Cannot get ZNP socket descriptor");
        free(loop);
        return 1;
    }
    status = uv_poll_init(loop, &znp_poll, znp_fd);
    if(status < 0)
    {
        LOG_ERR("Cannot add ZNP socket descriptor (%d) to main event loop : %s (%s)",
                znp_fd, uv_err_name(status), uv_strerror(status));
        free(loop);
        return 1;
    }

    uv_poll_start(&znp_poll, UV_READABLE, znp_poll_cb);

    uv_signal_init(loop, &sig_int);

    uv_signal_start(&sig_int, signal_handler, SIGINT);

    zg_zll_init(_init_complete_cb);
    LOG_INF("Starting main loop");
    uv_run(loop, UV_RUN_DEFAULT);


    LOG_INF("Quitting application");
    uv_poll_stop(&znp_poll);
    znp_shutdown();
    exit(0);
}
