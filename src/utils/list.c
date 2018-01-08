#include <stdlib.h>
#include "utils.h"
#include "list.h"

#define ZG_LIST_DATA_GET(x)         x ? x->data : NULL
#define ZG_LIST_NEXT(x)             x ? x->next : NULL

/********************************
 *          INTERNALS           *
 *******************************/

static ZgList *_last(ZgList *l)
{
    ZgList *tmp = l;

    if(!tmp)
        return NULL;

    while(tmp->next)
        tmp = tmp->next;

    return tmp;
}

/********************************
 *             API              *
 *******************************/

ZgList *zg_list_append(ZgList *list, void *data)
{
    ZgList *new = calloc(1, sizeof(ZgList));
    if(!new)
    {
        return NULL;
    }
    new->data = data;

    if(!list)
        return new;
    else
        _last(list)->next = new;
    return list;
}

ZgList *zg_list_remove(ZgList *list, ZgList *remove)
{
    ZgList *prev = list, *next = list;
    if(!list || !remove)
        return NULL;

    while(next != NULL && next != remove)
    {
        prev = next;
        next = next->next;
    }

    if(next == remove)
    {
        next = remove->next;
        free(remove);
        prev->next = next;
    }

    return list;
}

void zg_list_free(ZgList *list, ZgListFreeCb cb)
{
    ZgList *tmp = list;

    while(list != NULL)
    {
        tmp = ZG_LIST_NEXT(list);
        if(cb)
            cb(list->data);
        free(list);
        list = tmp;
    }
}

void *zg_list_data_get(ZgList *list)
{
    return ZG_LIST_DATA_GET(list);
}

void *zg_list_next(ZgList *list)
{
    return ZG_LIST_NEXT(list);
}

ZgList *zg_list_last(ZgList *list)
{
    if(!list)
        return NULL;
    return _last(list);
}
