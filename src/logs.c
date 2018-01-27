#include "logs.h"

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
