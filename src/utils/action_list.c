#include <stdlib.h>
#include "utils.h"
#include "action_list.h"

ZgAl *zg_al_create(ZgAlState *states, int nb_states)
{
    ZgAl *result = NULL;

    if(!states || nb_states <= 0)
    {
        return NULL;
    }
    result = calloc(1, sizeof(ZgAl));
    if(result)
    {
        result->states = states;
        result->nb_states = nb_states;
        result->current_state_index = -1;
    }
    return result;
}

void zg_al_destroy(ZgAl *al)
{
    ZG_VAR_FREE(al);
}

int zg_al_continue(ZgAl *al)
{
    int *index = &(al->current_state_index);
    int result = 1;
    if(al && *index + 1 < al->nb_states)
    {
        (*index)++;
        al->states[*index].entry_func(al->states[*index].data);
        result = 0;
    }
    return result;
}



