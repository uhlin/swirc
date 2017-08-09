#ifndef IO_LOOP_H
#define IO_LOOP_H

#define BOLD_ALIAS '\025'

typedef void (*CMD_HANDLER_FN)(const char *);

extern bool g_io_loop;

char *get_prompt          (void);
void  unget_string        (char *);
void  transmit_user_input (const char *win_label, const char *input);
void  enter_io_loop       (void);
void  cmd_help            (const char *data);

#endif
