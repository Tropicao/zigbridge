#ifndef INTERFACES_H
#define INTERFACES_H

#include <stdio.h>
#include <stdlib.h>

#define ZG_INTERFACES_MAX_INTERFACE_NAME_SIZE   32
#define ZG_INTERFACES_MAX_COMMAND_STRING_LEN    128

#define CALLOC_COMMAND_OBJ_RET(x, y)            do {\
                                                    x = calloc(1, sizeof(ZgInterfacesCommandObject));\
                                                    if(!x) {\
                                                        fprintf(stderr, "Cannot create Command object\n");\
                                                        return y;\
                                                }} while(0)

/* Callback type used to dispatch an event on interfaces that support events */
typedef void (*event_cb_t)(void *data, int size);

/* List of available commands from external client */
typedef enum
{
    ZG_INTERFACES_COMMAND_GET_VERSION,
    ZG_INTERFACES_COMMAND_OPEN_NETWORK,
    ZG_INTERFACES_COMMAND_TOUCHLINK,
    ZG_INTERFACES_COMMAND_GET_DEVICE_LIST,
    ZG_INTERFACES_COMMAND_MAX_ID
} ZgInterfacesCommandId;

/* List of availabel events to send to external clients */
typedef enum
{
    ZG_INTERFACES_NEW_DEVICE
} ZgInterfacesEvent;

typedef struct
{
    char command_string[ZG_INTERFACES_MAX_COMMAND_STRING_LEN];
    void *data;
    int len;
}ZgInterfacesCommandObject;

typedef struct
{
    int status;
    void *data;
    int len;
}ZgInterfacesAnswerObject;

typedef struct
{
    const char name[ZG_INTERFACES_MAX_INTERFACE_NAME_SIZE];
    event_cb_t event_cb;
} ZgInterfacesInterface;

/**
 * \brief Register a new Zigbridge input/output interface
 * When registering itself, the new interface must provide an allocated ZgInterfacesInterface structure to plug itself
 * properly into Zigbridge
 * \param interface An allocated ZgInterfacesInterface structure
 */
void zg_interfaces_register_new_interface(ZgInterfacesInterface *interface);

/**
 * \brief Function used to pass a received command to Zigbridge.
 * \param command The structure representing the command and data to process by Zibridge
 * \return A newly allocated object containing the answer to send to the lcient who has emitted the command. The object, if not NULL,
 * must be freed by the caller after it has been sent
 */
ZgInterfacesAnswerObject *zg_interfaces_process_command(ZgInterfacesInterface *interface, ZgInterfacesCommandObject *command);

void zg_interfaces_free_command_object(ZgInterfacesCommandObject *obj);

void zg_interfaces_free_answer_object(ZgInterfacesAnswerObject *obj);

void zg_interfaces_init();

void zg_interfaces_shutdown();

#endif
