#ifndef ZG_SM_H
#define ZG_SM_H

/**
 * @brief Basic state machine implementation
 *
 * This implementation only provides a way to simply execute a list of actions
 * asynchronously (it is not a real "state machine"). The user only has to
 * create a new state machine with zg_sm_create, providing all the fuinctions to
 * be executed. The function list can then be run step by step using
 * zg_sm_continue. When not needed anymore, the user has to delete the state
 * machine with zg_sm_destroy
 */

typedef void (*ZgSmFunc)(void (*cb)(void));
typedef struct
{
    ZgSmFunc entry_func;
    void *data;
}ZgSmState;

typedef struct
{
    int current_state_index;
    int nb_states;
    ZgSmState *states;
} ZgSm;

/**
 * @brief Function used to create a state machine
 *
 * @param nb_func The number of functions/states provided
 * @param The entry functions associated to each step
 * @return A newly allocated state machine
 */
ZgSm *zg_sm_create(ZgSmState *states,int nb_states);

/**
 * @brief Function used to destroy a state machine
 *
 * @param sm The state machine to free
 */
void zg_sm_destroy(ZgSm *sm);

/**
 * @brief Function used to make the state machine go to next state
 *
 * @param sm The state machine to make progress
 * @return 0 if action has been executed, 0 if state machine cannot progress
 * anymore (no more action available)
 */
int zg_sm_continue(ZgSm *sm);

#endif

