#ifndef INTERPRETER_H
#define INTERPRETER_H

enum setting_type {
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_STRING
};

typedef bool (*Interpreter_vFunc)(const char *);
typedef int (*Interpreter_instFunc)(const char *, const char *);

struct Interpreter_in {
    char			*path;
    char			*line;
    long int			 line_num;
    Interpreter_vFunc		 validator_func;
    Interpreter_instFunc	 install_func;
};

__SWIRC_BEGIN_DECLS
void Interpreter(const struct Interpreter_in *);
__SWIRC_END_DECLS

#include "dataClassify.h" /* sw_isspace() */

static SW_INLINE void
adv_while_isspace(const char **ptr)
{
    while (sw_isspace(**ptr)) {
	(*ptr)++;
    }
}

#endif
