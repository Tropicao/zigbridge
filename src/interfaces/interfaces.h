#ifndef INTERFACES_H
#define INTERFACES_H

#define MAX_INTERFACE_NAME_SIZE 32

/* Callback type used to dispatch an event on interfaces that support events */
typedef void (*event_cb_t)(void *data, int size);

/* List of available commands from external client */
typedef enum
{
    ZG_INTERFACES_COMMAND_GET_VERSION,
    ZG_INTERFACES_COMMAND_OPEN_NETWORK,
    ZG_INTERFACES_COMMAND_START_TOUCHLINK,
    ZG_INTERFACES_COMMAND_STOP_TOUCHLINK,
    ZG_INTERFACES_COMMAND_GET_DEVICE_LIST
} ZgInterfacesCommand;

/* List of availabel events to send to external clients */
typedef enum
{
    ZG_INTERFACES_NEW_DEVICE
} ZgInterfacesEvent;

typedef struct
{
    ZgInterfacesCommand command;
    void *data;
    int len;
}ZgInterfacesCommandObject;

typedef struct
{
    void *data;
    int len;
}ZgInterfacesAnswerObject;

typedef struct
{
    const char name[MAX_INTERFACE_NAME_SIZE];
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

void zg_interfaces_init();

void zg_interfaces_shutdown();

#endif
