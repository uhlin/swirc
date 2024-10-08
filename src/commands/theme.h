#ifndef SRC_COMMANDS_THEME_H_
#define SRC_COMMANDS_THEME_H_

#include <stdio.h> /* FILE */

#include "../textBuffer.h"

typedef enum {
	FOPEN_FAILED,
	PARSE_ERROR,
	READ_DB_OK,
	READ_INCOMPLETE
} read_result_t;

#define MAX_NO_THEMES	101

#define THEME_INFO_FOREACH(ar_p)\
	for (ar_p = &theme_info_array[0];\
	    ar_p < &theme_info_array[MAX_NO_THEMES];\
	    ar_p++)

typedef struct tagTHEME_INFO {
	char *filename;
	char *version;
	char *author;
	char *email;
	char *timestamp;
	char *comment;
} THEME_INFO, *PTHEME_INFO;

/*lint -sem(get_list_of_matching_theme_cmds, r_null) */

__SWIRC_BEGIN_DECLS
void		cmd_theme(const char *);
PTEXTBUF	get_list_of_matching_theme_cmds(const char *);
bool		get_next_line_from_file(FILE *, char **);
void		url_to_file(const char *, const char *);
__SWIRC_END_DECLS

#endif
