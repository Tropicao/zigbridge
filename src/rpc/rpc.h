#ifndef ZG_RPC_H
#define ZG_RPC_H

#include <stdint.h>

/**
 * \brief Initialize the communication medium to the ZNP.
 * \param device A string containing the name of serial device to open. If not
 * provided, it will try to open the device defined in configuration
 * \return 0 if init is successfull, otherwise 1;
 */
uint8_t zg_rpc_init(char *device);

/**
 * \brief Close the ZNP medium.
 */
void zg_rpc_shutdown(char *device);

/**
 * \brief Manually read data on znp medium. must be used each time that znp fd
 * notify data available.
 */
void zg_rpc_read(void);

/**
 * \brief Write data to ZNP medium
 * \return 0 if success, otherwise 1
 */
uint8_t zg_rpc_write(uint8_t *data, int len);

/**
 * \brief Return the filed descriptor of ZNP medium
 * \return A valid fd, or -1 if not opened.
 */
int zg_rpc_get_fd(void);


#endif

