#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <dbgPrint.h>
#include <pthread.h>
#include <uv.h>
#include "app.h"

#define SERIAL_DEVICE   "/dev/ttyACM0"

int quit = 0;
uv_loop_t *loop = NULL;

static void signal_handler(uv_signal_t *handle __attribute__((unused)), int signum __attribute__((unused)))
{
    log_inf("Application received SIGINT");
    quit = 1;
    uv_stop(loop);
}

static void *rpc_task(void *arg __attribute__((unused)))
{
    log_linf("Starting RPC task");
    rpcInitMq();
    while(quit == 0)
    {
        rpcProcess();
    }
    rpcClose();
    log_inf("Device %s closed", SERIAL_DEVICE);
    log_linf("Stopping RPC task");
    return NULL;
}


int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    pthread_t rpc_thread;
    uv_signal_t sig_int;
    int serialPortFd = 0;

    loop = calloc(1, sizeof(uv_loop_t));
    if(!loop)
    {
        log_inf("Cannot allocate memory for main loop structure");
        exit(1);
    }
    uv_loop_init(loop);
    uv_signal_init(loop, &sig_int);

	serialPortFd = rpcOpen(SERIAL_DEVICE, 0);
	if (serialPortFd == -1)
	{
		log_inf("could not open serial port");
		exit(-1);
	}
    log_inf("Device %s opened", SERIAL_DEVICE);

    uv_signal_start(&sig_int, signal_handler, SIGINT);
    pthread_create(&rpc_thread, NULL, rpc_task, NULL);
    uv_async_init(loop, &state_flag, state_machine_cb);

    app_register_callbacks();
    reset_dongle();
    log_inf("Starting main loop");
    uv_run(loop, UV_RUN_DEFAULT);

    log_inf("Quitting application");
    exit(0);
}
