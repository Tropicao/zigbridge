#ifndef ZG_UTILS_H
#define ZG_UTILS_H

#define ENSURE_SINGLE_INIT(x)       do{ \
                                        if(++x > 1) \
                                            return 0; \
                                    }while(0);

#define ENSURE_SINGLE_SHUTDOWN(x)   do{ \
                                    if(x < 1) \
                                        return; \
                                    if(--x != 0) \
                                        return; \
                                    } while(0);

#define ZG_VAR_FREE(x)              { if(x) {free(x); x = NULL;}}

#define ZG_CALLOC_RET(x, y)         do { if (!x)                       \
                                        x = calloc(1, sizeof(y));   \
                                        if(!x)                      \
                                        {                           \
                                            return;                 \
                                        }                           \
                                    } while(0);

#define ZG_CALLOC_ERR_CODE(x, y, z) do { if (!x)                       \
                                        x = calloc(1, sizeof(y));   \
                                        if(!x)                      \
                                        {                           \
                                            return z;               \
                                        }                           \
                                    } while(0);
#endif

