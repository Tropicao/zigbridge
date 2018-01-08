#ifndef ZG_LIST_H
#define ZG_LIST_H

/********************************
 *          Data types          *
 *******************************/

typedef struct ZgList
{
    void *data;
    struct ZgList *next;
} ZgList;

typedef void (*ZgListFreeCb)(void *data);

/********************************
 *             API              *
 *******************************/

ZgList *zg_list_append(ZgList *list, void *data);
ZgList *zg_list_remove(ZgList *list, ZgList *remove);
void zg_list_free(ZgList *list, ZgListFreeCb cb);
void *zg_list_data_get(ZgList *list);
void *zg_list_next(ZgList *list);
ZgList *zg_list_last(ZgList *list);

/********************************
 *    Constants and macros      *
 *******************************/

#define ZG_LIST_FOREACH(list, l, data)\
    for(    l = list,\
            data = zg_list_data_get(l);\
            l;\
            l = zg_list_next(l),\
            data = zg_list_data_get(l)\
       )

#define ZG_LIST_FREE(l, data)\
    for(    data = zg_list_data_get(l);\
            l;\
            l = zg_list_remove(l, l),\
            data = zg_list_data_get(l)\
       )


#endif

