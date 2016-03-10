#ifndef INIT_ONCE_H
#define INIT_ONCE_H

enum {
    ONCE_INITIALIZER,
    ONCE_EXCHANGE_VALUE
};

typedef volatile long int init_once_t;
typedef void (*PTR_TO_INIT_ROUTINE)(void);

int init_once(init_once_t *once_control, PTR_TO_INIT_ROUTINE init_routine);

#endif
