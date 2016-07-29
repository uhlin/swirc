#ifndef IO_LOOP_H
#define IO_LOOP_H

typedef void (*CMD_HANDLER_FN)(const char *);

extern bool g_io_loop;

void transmit_user_input (const char *win_label, const char *input);
void enter_io_loop       (void);
void cmd_help            (const char *data);

#endif
