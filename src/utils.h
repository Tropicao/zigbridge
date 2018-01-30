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

#define ZG_VAR_FREE(x)  { if(x) {free(x); x = NULL;}}

#endif

