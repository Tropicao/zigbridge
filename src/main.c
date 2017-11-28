#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <dbgPrint.h>

#define SERIAL_DEVICE   "/dev/ttyACM0"

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	int serialPortFd = rpcOpen(SERIAL_DEVICE, 0);
	if (serialPortFd == -1)
	{
		dbg_print(PRINT_LEVEL_ERROR, "could not open serial port\n");
		exit(-1);
	}
    dbg_print(PRINT_LEVEL_INFO, "Device %s opened\n", SERIAL_DEVICE);
    rpcClose();
    dbg_print(PRINT_LEVEL_INFO, "Device %s closed\n", SERIAL_DEVICE);
    exit(0);
}
