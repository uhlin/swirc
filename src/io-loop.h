#ifndef IO_LOOP_H
#define IO_LOOP_H

typedef void (*CMD_HANDLER_FN)(const char *);

extern bool g_io_loop;

void enter_io_loop(void);

#endif
