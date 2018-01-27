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
#define ZG_COLOR_YELLOW         "\033[33;1m"

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


#endif

