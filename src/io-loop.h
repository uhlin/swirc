#ifndef IO_LOOP_H
#define IO_LOOP_H

#include "textBuffer.h"

#define BOLD_ALIAS '\025'
#define MAX_PBB 2705

typedef void (*CMD_HANDLER_FN)(const char *);

#ifdef __cplusplus
extern "C" {
#endif

extern wchar_t g_push_back_buf[MAX_PBB];
extern bool g_io_loop;

/*lint -sem(get_list_of_matching_commands, r_null) */

PTEXTBUF	 get_list_of_matching_commands(const char *search_var);
char		*get_prompt(void);
void		 cmd_help(const char *);
void		 enter_io_loop(void);
void		 transmit_user_input(const char *, const char *input);

#ifdef __cplusplus
}
#endif

#endif
