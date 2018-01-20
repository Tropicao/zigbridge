#ifndef ZG_SM_H
#define ZG_SM_H

#include <stdint.h>

#define ZG_SM_NAME_MAX_LEN              256

typedef uint8_t ZgSmState;
typedef uint8_t ZgSmStateNb;
typedef uint8_t ZgSmEvent;
typedef uint8_t ZgSmTransitionNb;
typedef void (*ZgSmEntryFunc)(void);

typedef struct
{
    ZgSmState state;
    ZgSmEvent event;
    ZgSmState new_state;
} ZgSmTransitionData;

typedef struct
{
    ZgSmState state;
    ZgSmEntryFunc func;
} ZgSmStateData;

typedef struct
{
    char *name;
    ZgSmStateData *states;
    ZgSmStateNb nb_states;
    ZgSmTransitionData *transitions;
    ZgSmTransitionNb nb_transitions;
    ZgSmState current_state;
} ZgSm;

ZgSm *zg_sm_create( const char *name,
                    ZgSmStateData *states,
                    ZgSmStateNb nb_states,
                    ZgSmTransitionData *transitions,
                    ZgSmTransitionNb nb_transitions);
uint8_t zg_sm_start(ZgSm *sm);
void zg_sm_stop(ZgSm *sm);
void zg_sm_send_event(ZgSm *sm, ZgSmEvent event);



#endif

