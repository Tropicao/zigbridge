#include <stdlib.h>
#include <string.h>
#include <znp.h>
#include "aps.h"
#include "mt_af.h"

/********************************
 *          Constants           *
 *******************************/

/* APS Header format */
#define APS_HEADER_SIZE           3
#define INDEX_FCS               0x0
#define INDEX_TRANS_SEQ_NUM     0x1
#define INDEX_COMMAND           0x2

#define APS_DEFAULT_FRAME_CONTROL               0x11

/********************************
 *          Local variables     *
 *******************************/

static uint8_t _transaction_sequence_number = 0;
static SyncActionCb _current_cb = NULL;

/********************************
 *          Internal            *
 *******************************/

static void _af_data_request_ext_cb(void)
{
    _transaction_sequence_number++;
    if(_current_cb)
    {
        _current_cb();
        _current_cb = NULL;
    }
}

/********************************
 *          Internal            *
 *******************************/

uint8_t _build_frame_control()
{
    return APS_DEFAULT_FRAME_CONTROL;
}

void zg_aps_send_data(  uint16_t dst_addr,
                        uint16_t dst_pan,
                        uint8_t src_endpoint,
                        uint8_t dst_endpoint,
                        uint16_t cluster,
                        uint8_t command,
                        void *data,
                        int len,
                        SyncActionCb cb)
{
    uint8_t *aps_data = NULL;
    int aps_data_len;
    if(len ==0 || !data)
    {
        LOG_ERR("Cannot send AF_DATA_REQUEST_EXT (%s)", data ? "length is invalid":"no data provided");
        return;
    }

    aps_data_len = APS_HEADER_SIZE + len;
    aps_data = calloc(aps_data_len, sizeof(uint8_t));
    if(!aps_data)
    {
        LOG_CRI("Cannot allocate memory for AF request");
        return;
    }

    if(cb)
        _current_cb = cb;

    aps_data[INDEX_FCS] = _build_frame_control();
    aps_data[INDEX_TRANS_SEQ_NUM] = _transaction_sequence_number;
    aps_data[INDEX_COMMAND] = command;
    memcpy(aps_data+APS_HEADER_SIZE, data, len);

    mt_af_send_data_request_ext(dst_addr,
            dst_pan,
            src_endpoint,
            dst_endpoint,
            cluster,
            aps_data_len,
            aps_data,
            _af_data_request_ext_cb);
    free(aps_data);
}

