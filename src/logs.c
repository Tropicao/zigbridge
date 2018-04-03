#include "logs.h"

static ZNPStatusString _status_list[] =
{
    {ZSUCCESS , "ZNP COMMAND SUCCESS"},
    {ZFAILURE , "ZNP failure"},
    {ZINVALIDPARAMETER , "ZNP invalid parameter"},
    {NV_ITEM_UNINIT, "ZNP NV item is not intialized"},
    {NV_OPER_FAILED , "ZNP NV operation is not valid"},
    {NV_BAD_ITEM_LEN , "ZNP NV item length is invalid"},
    {ZMEMERROR , "ZNP memory error"},
    {ZBUFFERFULL , "ZNP buffer is full"},
    {ZUNSUPPORTEDMODE , "ZNP unsupported mode"},
    {ZMACMEMERROR , "ZNP MAC memory error"},
    {ZDOINVALIDREQUESTTYPE , "ZNP ZDO request is invalid"},
    {ZDODEVICENOTFOUND , "ZNP ZDO device not found"},
    {ZDOINVALIDENDPOINT , "ZNP ZDO request on invalid endpoint"},
    {ZDOUNSUPPORTED , "ZNP ZDO command is not supported"},
    {ZDOTIMEOUT , "ZNP ZDO timeout"},
    {ZDONOMATCH , "ZNP ZDO found no match"},
    {ZDOTABLEFULL , "ZNP ZDO table is full"},
    {ZDONOBINDENTRY , "ZNP ZDO found no bind entry"},
    {ZSECNOKEY , "ZNP found no security key"},
    {ZSECMAXFRMCOUNT , "ZNP security max frame count reached"},
    {ZAPSFAIL , "ZNP APS failed"},
    {ZAPSTABLEFULL , "ZNP APS table is full"},
    {ZAPSILLEGALREQUEST , "ZNP APS illegal request"},
    {ZAPSINVALIDBINDING , "ZNP APS binding is invalid"},
    {ZAPSUNSUPPORTEDATTRIB , "ZNP APS unsupported attribute"},
    {ZAPSNOTSUPPORTED , "ZNP APS operation not supported"},
    {ZAPSNOACK , "ZNP APS no acknowledgement received"},
    {ZAPSDUPLICATEENTRY , "ZNP APS duplicate entry"},
    {ZAPSNOBOUNDDEVICE , "ZNP APS no bound device found"},
    {ZNWKINVALIDPARAM , "ZNP APS invalid parameter"},
    {ZNWKINVALIDREQUEST , "ZNP APS invalid request"},
    {ZNWKNOTPERMITTED , "ZNP APS operation not permitted"},
    {ZNWKSTARTUPFAILURE , "ZNP NWK startup failure"},
    {ZNWKTABLEFULL , "ZNP NWK Table is full"},
    {ZNWKUNKNOWNDEVICE , "ZNP NWK unkown device"},
    {ZNWKUNSUPPORTEDATTRIBUTE , "ZNP NWM unsupported attribute"},
    {ZNWKNONETWORKS , "ZNP NWK no network available"},
    {ZNWKLEAVEUNCONFIRMED , "ZNP NWK Leave command unconfirmed"},
    {ZNWKNOACK , "ZNP NWK No Acknowledgement received"},
    {ZNWKNOROUTE , "ZNP NWK no route found"},
    {ZMACNOACK , "ZNP MAC no acknowledgemeent received"}
};
static uint8_t _status_list_len = sizeof(_status_list)/sizeof(ZNPStatusString);

void zg_logs_init()
{
    if(!eina_init())
    {
        fprintf(stderr, "Error while intializing EINA");
        return;
    }
}

int zg_logs_domain_register(const char *name,  const char *color)
{
    return eina_log_domain_register(name, color);
}

void zg_logs_shutdown()
{

}

const char *zg_logs_znp_strerror(ZNPStatus status)
{
    int index = 0;
    for(index = 0; index < _status_list_len; index++)
    {
        if(_status_list[index].status == status)
            return _status_list[index].string;
    }
    return NULL;
}

