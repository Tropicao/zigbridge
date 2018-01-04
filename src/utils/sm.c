#include <stdlib.h>
#include <znp.h>
#include "sm.h"

ZgSm *zg_sm_create(ZgSmState *states, int nb_states)
{
    ZgSm *result = NULL;

    if(!states || nb_states <= 0)
    {
        LOG_DBG("Cannot create state machine : data is empty");
        return NULL;
    }
    result = calloc(1, sizeof(ZgSm));
    if(result)
    {
        result->states = states;
        result->nb_states = nb_states;
        result->current_state_index = -1;
    }
    return result;
}

void zg_sm_destroy(ZgSm *sm)
{
    if(sm)
        free(sm);
}

int zg_sm_continue(ZgSm *sm)
{
    int *index = &(sm->current_state_index);
    int result = 1;
    if(sm && *index + 1 < sm->nb_states)
    {
        (*index)++;
        LOG_DBG("Executing new action (%d/%d)", (*index) + 1, sm->nb_states);
        sm->states[*index].entry_func(sm->states[*index].data);
        result = 0;
    }
    return result;
}



