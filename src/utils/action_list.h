#ifndef ZG_AL_H

#define ZG_AL_H

/**
 * @brief Basic state machine implementation
 *
 * This implementation only provides a way to simply execute a list of actions
 * asynchronously (it is not a real "state machine"). The user only has to
 * create a new state machine with zg_al_create, providing all the fuinctions to
 * be executed. The function list can then be run step by step using
 * zg_al_continue. When not needed anymore, the user has to delete the state
 * machine with zg_al_destroy
 */

typedef void (*ZgAlFunc)(void (*cb)(void));
typedef struct
{
    ZgAlFunc entry_func;
    void *data;
}ZgAlState;

typedef struct
{
    int current_state_index;
    int nb_states;
    ZgAlState *states;
} ZgAl;

/**
 * @brief Function used to create a state machine
 *
 * @param nb_func The number of functions/states provided
 * @param The entry functions associated to each step
 * @return A newly allocated state machine
 */
ZgAl *zg_al_create(ZgAlState *states,int nb_states);

/**
 * @brief Function used to destroy a state machine
 *
 * @param al The state machine to free
 */
void zg_al_destroy(ZgAl *al);

/**
 * @brief Function used to make the state machine go to next state
 *
 * @param al The state machine to make progress
 * @return 0 if action has been executed, 0 if state machine cannot progress
 * anymore (no more action available)
 */
int zg_al_continue(ZgAl *al);

#endif

