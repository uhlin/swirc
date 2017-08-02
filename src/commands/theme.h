#ifndef CMD_THEME_H
#define CMD_THEME_H

typedef enum {
    FOPEN_FAILED,
    PARSE_ERROR,
    READ_DB_OK,
    READ_INCOMPLETE
} read_result_t;

#define MAX_NO_THEMES 101

#define THEME_INFO_FOREACH(ar_p) \
	for (ar_p = &theme_info_array[0]; ar_p < &theme_info_array[MAX_NO_THEMES]; ar_p++)

typedef struct tagTHEME_INFO {
    char *filename;
    char *version;
    char *author;
    char *email;
    char *timestamp;
    char *comment;
} THEME_INFO, *PTHEME_INFO;

void cmd_theme(const char *);

#endif
