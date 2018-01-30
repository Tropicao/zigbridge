#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "types.h"
#include "rpc.h"
#include "logs.h"
#include "utils.h"
#include "conf.h"

/********************************
 *          Constants           *
 *******************************/

#define RPC_SOF                     0xFE
#define RPC_SOF_INDEX               0
#define RPC_SOF_SIZE                1

#define RPC_DATA_LEN_INDEX          RPC_SOF_INDEX + RPC_SOF_SIZE
#define RPC_DATA_LEN_SIZE           1

#define RPC_CMD0_INDEX              RPC_DATA_LEN_INDEX + RPC_DATA_LEN_SIZE
#define RPC_CMD0_SIZE               1
#define RPC_CMD0_TYPE_MASK          0xE0
#define RPC_CMD0_SUBSYS_MASK        0x1F

#define RPC_CMD1_INDEX              RPC_CMD0_INDEX + RPC_CMD0_SIZE
#define RPC_CMD1_SIZE               1

#define RPC_DATA_INDEX              RPC_CMD1_INDEX + RPC_CMD1_SIZE
#define RPC_MAX_DATA_SIZE           250

#define RPC_FCS_INDEX(x)            RPC_DATA_INDEX + x
#define RPC_FCS_SIZE                1

#define RPC_FRAME_MAX_SIZE          RPC_SOF_SIZE \
                                    + RPC_DATA_LEN_SIZE \
                                    + RPC_CMD0_SIZE \
                                    + RPC_CMD1_SIZE \
                                    + RPC_MAX_DATA_SIZE \
                                    + RPC_FCS_SIZE

#define FRAME_SIZE(x)               RPC_SOF_SIZE \
                                    + RPC_DATA_LEN_SIZE \
                                    + RPC_CMD0_SIZE \
                                    + RPC_CMD1_SIZE \
                                    + x \
                                    + RPC_FCS_SIZE

/********************************
 *          Structs             *
 *******************************/

typedef struct
{
    const char *name;
    ZgMtSubSys subsys;
    mt_subsys_cb_t cb;
} mt_subsys_t;

/********************************
 *       Local variables        *
 *******************************/

static const char *_device = NULL;
static int _znp_fd = -1;
static int _log_domain = -1;
static int _init_count = 0;
/* Callbacks table for MT subsystems */
static mt_subsys_t _mt_subsys_table[] = {
    {"RESERVED", ZG_MT_SUBSYS_RESERVED, NULL},
    {"SYS", ZG_MT_SUBSYS_SYS, NULL},
    {"MAC", ZG_MT_SUBSYS_MAC, NULL},
    {"NWK", ZG_MT_SUBSYS_NWK, NULL},
    {"AF", ZG_MT_SUBSYS_AF, NULL},
    {"ZDO", ZG_MT_SUBSYS_ZDO, NULL},
    {"SAPI", ZG_MT_SUBSYS_SAPI, NULL},
    {"UTIL", ZG_MT_SUBSYS_UTIL, NULL},
    {"DEBUG", ZG_MT_SUBSYS_DEBUG, NULL},
    {"APP_INT", ZG_MT_SUBSYS_APP_INT, NULL},
    {"APP_CONFIG", ZG_MT_SUBSYS_APP_CONFIG, NULL},
    {"GREENPOWER", ZG_MT_SUBSYS_GREENPOWER, NULL}
};

static uint8_t _mt_subsys_table_size = sizeof(_mt_subsys_table)/sizeof(mt_subsys_t);

/********************************
 *      Internal functions      *
 *******************************/

static uint8_t _compute_frame_fcs(uint8_t *buffer, uint8_t len)
{
    uint8_t res = 0;
    uint8_t index = 0;
    for(index = 0; index<len; index++)
        res ^= buffer[index];
    return res;
}

static void _process_rpc_frame(ZgMtMsg *msg)
{
    uint8_t index = 0;

    for(index = 0; index < _mt_subsys_table_size; index++)
    {
        if(_mt_subsys_table[index].subsys == msg->subsys &&
                _mt_subsys_table[index].cb)
        {
            _mt_subsys_table[index].cb(msg);
            break;
        }
    }
    if(index == _mt_subsys_table_size)
        WRN("No registered callback found for incoming RPC message");
}


static void _read_znp_data(void)
{
    uint8_t buffer[RPC_MAX_DATA_SIZE] = {0};
    uint8_t fcs_computed = 0x00;
    uint8_t len = 0;
    uint8_t bytes_read = 0;
    int8_t i;
    ZgMtMsg msg;

    if(_znp_fd < 0)
    {
        ERR("Cannot read data : ZNP medium not opened");
        return;
    }

    read(_znp_fd, buffer + RPC_SOF_INDEX, RPC_SOF_SIZE);
    if(buffer[RPC_SOF_INDEX] != RPC_SOF)
    {
        ERR("Error : invalid start of frame");
        goto znp_read_err;
    }

    if(read(_znp_fd, buffer + RPC_DATA_LEN_INDEX, RPC_DATA_LEN_SIZE) != RPC_DATA_LEN_INDEX)
    {
        ERR("Error reading Length field in incoming frame");
        goto znp_read_err;
    }
    if(read(_znp_fd, buffer + RPC_CMD0_INDEX, RPC_CMD0_SIZE) != RPC_CMD1_SIZE)
    {
        ERR("Error reading CMD0 field in incoming frame");
        goto znp_read_err;
    }

    if(read(_znp_fd, buffer + RPC_CMD1_INDEX, RPC_CMD1_SIZE) != RPC_CMD1_SIZE)
    {
        ERR("Error reading CMD1 field in incoming frame");
        goto znp_read_err;
    }
    len = buffer[RPC_DATA_LEN_INDEX];
    DBG("Data size is %d", len);
    bytes_read = read(_znp_fd, buffer + RPC_DATA_INDEX, len);
    if(bytes_read != len)
    {
        ERR("Error : did not manage to read %d bytes (only %d read)", len, bytes_read);
        goto znp_read_err;
    }

    read(_znp_fd, buffer + RPC_FCS_INDEX(len), RPC_FCS_SIZE);

    /* Display complete frame in debug log */
    for(i = 0; i < FRAME_SIZE(len); i++)
        DBG("Data %d : 0x%02X", i, buffer[i]);

    fcs_computed = _compute_frame_fcs(buffer + RPC_DATA_LEN_INDEX, RPC_DATA_LEN_SIZE + RPC_CMD0_SIZE + RPC_CMD1_SIZE + len);
    if(fcs_computed != buffer[RPC_FCS_INDEX(len)])
    {
        ERR("Error : invalid frame check (should be 0x%02X, got 0x%02X)", fcs_computed, buffer[RPC_FCS_INDEX(len)]);
        goto znp_read_err;
    }
    INF("ZNP has sent %d bytes", FRAME_SIZE(len));
    msg.type = buffer[RPC_CMD0_INDEX] & RPC_CMD0_TYPE_MASK;
    msg.subsys = buffer[RPC_CMD0_INDEX] & RPC_CMD0_SUBSYS_MASK;
    msg.cmd = buffer[RPC_CMD1_INDEX];
    msg.data = buffer + RPC_DATA_INDEX;
    msg.len = buffer[RPC_DATA_LEN_INDEX];

    _process_rpc_frame(&msg);
    return;

znp_read_err:
    DBG("Purging invalid ZNP data");
	tcflush(_znp_fd, TCIFLUSH);
}

/********************************
 *              API             *
 *******************************/

uint8_t zg_rpc_init(void)
{
    struct termios tio;
    const char *device = zg_conf_get_znp_device_path();

    ENSURE_SINGLE_INIT(_init_count);
    _log_domain = zg_logs_domain_register("zg_rpc", EINA_COLOR_BLUE);
    if(!device)
    {
        ERR("No device provided, abort");
        return 1;
    }

    INF("Opening ZNP device %s", device);
    _device = device;

    _znp_fd = open(_device, O_RDWR|O_NOCTTY);
    if(_znp_fd < 0)
    {
        ERR("Cannot open device %s : %s", _device, strerror(errno));
        return 1;
    }
    DBG("ZNP medium opened (fd %d)", _znp_fd);

	tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR & ~ICRNL;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;

	tcflush(_znp_fd, TCIFLUSH);
	tcsetattr(_znp_fd, TCSANOW, &tio);
    INF("RPC module initialized");

    return 0;
}


void zg_rpc_shutdown(void)
{
    ENSURE_SINGLE_SHUTDOWN(_init_count);
    if(_znp_fd > 0)
    {
        INF("Closing ZNP medium");
        tcflush(_znp_fd, TCIFLUSH);
        close(_znp_fd);
        _znp_fd = -1;
    }

}

void zg_rpc_read(void)
{
    _read_znp_data();
}

uint8_t zg_rpc_write(ZgMtMsg *msg)
{
    uint8_t buf[RPC_FRAME_MAX_SIZE] = {0};
    uint16_t total_size = 0;
    uint8_t i = 0;

    if(_znp_fd < 0)
    {
        ERR("Cannot send data to ZNP, medium not initialized");
        return 1;
    }
    if(!msg)
    {
        ERR("Cannot send data to ZNP, message object is empty");
        return 1;
    }

    if(msg->len > RPC_MAX_DATA_SIZE)
    {
        ERR("Cannot send data to ZNP, data is too large (%d)", msg->len);
        return 1;
    }

    total_size = FRAME_SIZE(msg->len);
    buf[RPC_SOF_INDEX] = RPC_SOF;
    buf[RPC_DATA_LEN_INDEX] = msg->len;
    buf[RPC_CMD0_INDEX] = msg->type|msg->subsys;
    buf[RPC_CMD1_INDEX] = msg->cmd;
    if(msg->len && msg->data)
        memcpy(buf+RPC_DATA_INDEX, msg->data, msg->len);
    buf[RPC_FCS_INDEX(msg->len)] = _compute_frame_fcs(buf + RPC_DATA_LEN_INDEX,
            RPC_DATA_LEN_SIZE + RPC_CMD0_SIZE + RPC_CMD1_SIZE + msg->len);

    INF("Writing %d bytes to ZNP", total_size);
    for(i = 0; i < total_size; i++)
        DBG("Data %d : 0x%02X", i, buf[i]);

    if(write(_znp_fd, buf, total_size) != total_size)
    {
        ERR("Error writing data to ZNP medium");
        return 1;
    }
    tcflush(_znp_fd, TCOFLUSH);
    return 0;
}

int zg_rpc_get_fd(void)
{
    return _znp_fd;
}

void zg_rpc_subsys_cb_set(ZgMtSubSys subsys, mt_subsys_cb_t cb)
{
    int index = 0;
    for(index = 0; index < _mt_subsys_table_size; index++)
    {
        if(_mt_subsys_table[index].subsys == subsys)
        {
            INF("Registering new callback for %s subsystem", _mt_subsys_table[index].name);
            _mt_subsys_table[index].cb = cb;
            break;
        }
    }
    if(index == _mt_subsys_table_size)
        WRN("Cannot register callback : no subsystem with id %d found", subsys);
}
