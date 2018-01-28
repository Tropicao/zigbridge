#ifndef ZG_LOGS_H
#define ZG_LOGS_H

#include <Eina.h>
#include <stdarg.h>

/* Make sure that our logging macro are not already defined elsewhere */
#ifdef CRI
#undef CRI
#endif
#ifdef ERR
#undef ERR
#endif
#ifdef WRN
#undef WRN
#endif
#ifdef INF
#undef INF
#endif
#ifdef DBG
#undef DBG
#endif

#define CRI(...) EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)

#define ZG_COLOR_WHITE          "\033[37;1m"
#define ZG_COLOR_BLACK          "\033[30;47m"
#define ZG_COLOR_RED            "\033[31m"
#define ZG_COLOR_LIGHTRED       "\033[31;1m"
#define ZG_COLOR_GREEN          "\033[32m"
#define ZG_COLOR_LIGHTGREEN     "\033[32;1m"
#define ZG_COLOR_BLUE           "\033[34m"
#define ZG_COLOR_LIGHTBLUE      "\033[34;1m"
#define ZG_COLOR_YELLOW         "\033[33m"
#define ZG_COLOR_LIGHTYELLOW    "\033[33;1m"
#define ZG_COLOR_MAGENTA        "\033[35;1m"
#define ZG_COLOR_LIGHTMAGENTA   "\033[95;1m"
#define ZG_COLOR_CYAN           "\033[36;1m"
#define ZG_COLOR_LIGHTCYAN      "\033[96;1m"


typedef enum
{
    ZSUCCESS =                  0x00,
    ZFAILURE =                  0x01,
    ZINVALIDPARAMETER =         0x02,
    NV_ITEM_UNINIT =            0x09,
    NV_OPER_FAILED =            0x0a,
    NV_BAD_ITEM_LEN =           0x0c,
    ZMEMERROR =                 0x10,
    ZBUFFERFULL =               0x11,
    ZUNSUPPORTEDMODE =          0x12,
    ZMACMEMERROR =              0x13,
    ZDOINVALIDREQUESTTYPE =     0x80,
    ZDOINVALIDENDPOINT =        0x82,
    ZDOUNSUPPORTED =            0x84,
    ZDOTIMEOUT =                0x85,
    ZDONOMATCH =                0x86,
    ZDOTABLEFULL =              0x87,
    ZDONOBINDENTRY =            0x88,
    ZSECNOKEY =                 0xa1,
    ZSECMAXFRMCOUNT =           0xa3,
    ZAPSFAIL =                  0xb1,
    ZAPSTABLEFULL =             0xb2,
    ZAPSILLEGALREQUEST =        0xb3,
    ZAPSINVALIDBINDING =        0xb4,
    ZAPSUNSUPPORTEDATTRIB =     0xb5,
    ZAPSNOTSUPPORTED =          0xb6,
    ZAPSNOACK =                 0xb7,
    ZAPSDUPLICATEENTRY =        0xb8,
    ZAPSNOBOUNDDEVICE =         0xb9,
    ZNWKINVALIDPARAM =          0xc1,
    ZNWKINVALIDREQUEST =        0xc2,
    ZNWKNOTPERMITTED =          0xc3,
    ZNWKSTARTUPFAILURE =        0xc4,
    ZNWKTABLEFULL =             0xc7,
    ZNWKUNKNOWNDEVICE =         0xc8,
    ZNWKUNSUPPORTEDATTRIBUTE =  0xc9,
    ZNWKNONETWORKS =            0xca,
    ZNWKLEAVEUNCONFIRMED =      0xcb,
    ZNWKNOACK =                 0xcc,
    ZNWKNOROUTE =               0xcd,
    ZMACNOACK =                 0xe9,
} ZNPStatus;


typedef struct
{
    ZNPStatus status;
    const char *string;
}ZNPStatusString;

/**
 * \brief Initialize the software logging utility
 */
void zg_logs_init();

/**
 * \brief Register a new log domain for the current software session
 * \param name The named to be logged for the domain to be registered
 * \param color The color to be used on the domain name in the logs. Refer to
 * eina_log.h in efl source code to find available colors
 * \return The value of log domain index that has been registered
 */
int zg_logs_domain_register(const char *name,  const char *color);

/**
 * \brief Deinitialize the logging module
 */
void zg_logs_shutdown();

/**
 * \brief Get the string corresponding to the provided ZNP error code
 * \param status The error code to translate
 * \return A string describing the error hold by the error code
 */
const char *zg_logs_znp_strerror(ZNPStatus status);

#endif

