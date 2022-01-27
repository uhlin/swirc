#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdio.h> /* FILE */

/*
 * Set to 0 to turn off this feature.
 */
#define IGNORE_UNRECOGNIZED_IDENTIFIERS 1

#define MAXLINE 3200

enum setting_type {
	TYPE_BOOLEAN,
	TYPE_INTEGER,
	TYPE_STRING
};

typedef bool (*Interpreter_vFunc)(const char *);
typedef int (*Interpreter_instFunc)(const char *, const char *);

struct Interpreter_in {
	char *path;
	char *line;
	long int line_num;
	Interpreter_vFunc validator_func;
	Interpreter_instFunc install_func;
};

__SWIRC_BEGIN_DECLS
void	Interpreter(const struct Interpreter_in *);
void	Interpreter_processAllLines(FILE *, const char *, Interpreter_vFunc,
	    Interpreter_instFunc);
__SWIRC_END_DECLS

#include "dataClassify.h" /* sw_isspace() */

static inline void
adv_while_isspace(const char **ptr)
{
	while (sw_isspace(**ptr))
		(*ptr)++;
}

#endif
