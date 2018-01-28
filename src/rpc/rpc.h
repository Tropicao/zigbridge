#ifndef ZG_RPC_H
#define ZG_RPC_H

#include <stdint.h>

typedef enum
{
    ZG_MT_CMD_POLL = 0x00,
    ZG_MT_CMD_SREQ = 0x20,
    ZG_MT_CMD_AREQ = 0x40,
    ZG_MT_CMD_SRSP = 0x60
} ZgMtCmd;

typedef enum
{
    ZG_MT_SUBSYS_RESERVED =     0x00,
    ZG_MT_SUBSYS_SYS =          0x01,
    ZG_MT_SUBSYS_MAC =          0x02,
    ZG_MT_SUBSYS_NWK =          0x03,
    ZG_MT_SUBSYS_AF =           0x04,
    ZG_MT_SUBSYS_ZDO =          0x05,
    ZG_MT_SUBSYS_SAPI =         0x06,
    ZG_MT_SUBSYS_UTIL =         0x07,
    ZG_MT_SUBSYS_DEBUG =        0x08,
    ZG_MT_SUBSYS_APP_INT =      0x09,
    ZG_MT_SUBSYS_APP_CONFIG =   0x0F,
    ZG_MT_SUBSYS_GREENPOWER =   0x15,
} ZgMtSubSys;

typedef struct
{
    ZgMtCmd type;
    ZgMtSubSys subsys;
    uint8_t cmd;
    uint8_t *data;
    uint8_t len;
} ZgMtMsg;

typedef void (*znp_frame_cb_t)(uint8_t *buf, uint8_t len);
typedef void (*mt_subsys_cb_t)(ZgMtMsg *msg);

/**
 * \brief Initialize the communication medium to the ZNP.
 * \param device A string containing the name of serial device to open. If not
 * provided, it will try to open the device defined in configuration
 * \return 0 if init is successfull, otherwise 1;
 */
uint8_t zg_rpc_init(const char *device);

/**
 * \brief Close the ZNP medium.
 */
void zg_rpc_shutdown();

/**
 * \brief Manually read data on znp medium. must be used each time that znp fd
 * notify data available.
 */
void zg_rpc_read(void);

/**
 * \brief Write data to ZNP medium
 * \param cmd The command type between POLL, SREQ, AREQ or SRSP (cf
 * enumeration in rpc.h)
 * \param subsys The MT subystem targeted by the command, cf rpc.h for
 * enumeration
 * \brief id The command id, defined by the targeted subsystem
 * \param data The corresponding data to send
 * \param len The data buffer size. This value is only for data buffer, it does
 * not take cmd and subsys size in account.
 * \return 0 if success, otherwise 1
 */
uint8_t zg_rpc_write(ZgMtCmd cmd, ZgMtSubSys subsys, uint8_t id, uint8_t *data, int len);

/**
 * \brief Return the filed descriptor of ZNP medium
 * \return A valid fd, or -1 if not opened.
 */
int zg_rpc_get_fd(void);

/**
 * \brief Subscribe to RPC messages targeted to a specific MT subsystem
 *
 * This function allow to trigger a specific callback to process the messages
 * targeted to a subsystem defined by the MT protocol. If a callback is already
 * registered for the targeted MT subsystem, it will be overwritten.
 * \param subsys The MT subsystem to subscribe to (cf ZgMtSubSys enumeration for
 * possible values)
 * \param cb The callback to associate to targeted subsystem messages
 */
void zg_rpc_subsys_cb_set(ZgMtSubSys subsys, mt_subsys_cb_t cb);

#endif

