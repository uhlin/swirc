#ifndef IO_LOOP_H
#define IO_LOOP_H

#define BOLD_ALIAS '\025'

typedef void (*CMD_HANDLER_FN)(const char *);

extern wchar_t g_push_back_buf[2705];
extern bool g_io_loop;

char *get_prompt          (void);
void  cmd_help            (const char *data);
void  enter_io_loop       (void);
void  transmit_user_input (const char *win_label, const char *input);

#endif
