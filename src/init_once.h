#ifndef INIT_ONCE_H
#define INIT_ONCE_H

enum {
	ONCE_INITIALIZER,
	ONCE_EXCHANGE_VALUE
};

typedef volatile long int init_once_t;
typedef void (*PTR_TO_INIT_ROUTINE)(void);

__SWIRC_BEGIN_DECLS
int	init_once(init_once_t *, PTR_TO_INIT_ROUTINE);
__SWIRC_END_DECLS

#endif
