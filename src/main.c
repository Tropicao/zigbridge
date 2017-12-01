#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <dbgPrint.h>
#include <uv.h>

#define SERIAL_DEVICE   "/dev/ttyACM0"

int quit = 0;
uv_loop_t *loop = NULL;

static void signal_handler(uv_signal_t *handle __attribute__((unused)), int signum __attribute__((unused)))
{
    log_inf("Application received SIGINT");
    quit = 1;
    uv_stop(loop);
}

static void rpc_task(void *arg __attribute__((unused)))
{
    while(quit == 0)
    {
        rpcProcess();
    }
    log_dbg("End of RPC task");
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    uv_thread_t rpc_id;
    uv_signal_t sig_int;
    int serialPortFd = 0;

    loop = calloc(1, sizeof(uv_loop_t));
    if(!loop)
    {
        dbg_print(PRINT_LEVEL_ERROR, "Cannot allocate memory for main loop structure\n");
        exit(1);
    }
    uv_loop_init(loop);
    uv_thread_create(&rpc_id, rpc_task, NULL);
    uv_signal_init(loop, &sig_int);

	serialPortFd = rpcOpen(SERIAL_DEVICE, 0);
	if (serialPortFd == -1)
	{
		dbg_print(PRINT_LEVEL_ERROR, "could not open serial port\n");
		exit(-1);
	}
    dbg_print(PRINT_LEVEL_INFO, "Device %s opened\n", SERIAL_DEVICE);

    uv_thread_join(&rpc_id);
    uv_signal_start(&sig_int, signal_handler, SIGINT);
    uv_run(loop, UV_RUN_DEFAULT);

    log_inf("Quitting application\n");
    rpcClose();
    dbg_print(PRINT_LEVEL_INFO, "Device %s closed\n", SERIAL_DEVICE);
    exit(0);
}
