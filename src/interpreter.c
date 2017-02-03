/* Copyright (c) 2012-2017 Markus Uhlin <markus.uhlin@bredband.net>
   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
   AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE. */

#include "common.h"
#include "errHand.h"
#include "interpreter.h"
#include "libUtils.h"

/* Set to 0 to turn off this feature. */
#define IGNORE_UNRECOGNIZED_IDENTIFIERS 1

static const size_t	identifier_maxSize = 50;
static const size_t	argument_maxSize   = 480;

/**
 * Copy identifier
 */
static char *
copy_identifier(const char *id)
{
    size_t	 count    = identifier_maxSize;
    char	*dest_buf = malloc(count + 1);
    char	*dest;

    if (!dest_buf)
	err_exit(ENOMEM, "copy_identifier");
    else {
	dest = &dest_buf[0];
    }

    while ((sw_isalnum(*id) || *id == '_') && count > 1) {
	*dest++ = *id++, count--;
    }

    *dest = '\0';
    if (count == 1)
	err_quit("In copy_identifier: fatal: string was truncated!");
    return (dest_buf);
}

/**
 * Copy argument
 */
static char *
copy_argument(const char *arg)
{
    bool	 inside_arg = true;
    size_t	 count      = argument_maxSize;
    char	*dest_buf   = malloc(count + 1);
    char	*dest;

    if (!dest_buf)
	err_exit(ENOMEM, "copy_argument");
    else {
	dest = &dest_buf[0];
    }

    while (*arg && count > 1) {
	if (*arg == '\"') {
	    inside_arg = false;
	    break;
	}

	*dest++ = *arg++, count--;
    }

    *dest = '\0';

    if (inside_arg && count == 1)
	err_quit("In copy_argument: fatal: string was truncated!");

    if (inside_arg) {
	free(dest_buf);
	return (NULL);
    }

    return (dest_buf);
}

/**
 * Interpreter
 */
void
Interpreter(const struct Interpreter_in *in)
{
#define interpreter_message(string) \
    err_msg("%s:%ld: Parse Error: %s", in->path, in->line_num, string)
    const char	*ccp = &in->line[0];
    char	*id  = NULL;
    char	*arg = NULL;

    if (!sw_isalnum(*ccp) && *ccp != '_') {
	interpreter_message("Unexpected leading character");
	goto die;
    }

    id = copy_identifier(ccp);
    while (sw_isalnum(*ccp) || *ccp == '_') {
	ccp++;
    }

    adv_while_isspace(&ccp);
    if (*ccp++ != '=') {
	interpreter_message("Expected expression after identifier");
	goto die;
    }

    adv_while_isspace(&ccp);
    if (*ccp++ != '\"') {
	interpreter_message("Expected statement after expression");
	goto die;
    } else if ((arg = copy_argument(ccp)) == NULL) {
	interpreter_message("Lacks ending quote for the argument");
	goto die;
    } else {
	;
    }

    while (*ccp++ != '\"') {
	;
    }

    adv_while_isspace(&ccp);
    if (*ccp++ != ';') {
	interpreter_message("No line terminator");
	goto die;
    }

    adv_while_isspace(&ccp);
    if (*ccp && *ccp != '#') {
	interpreter_message("Implicit data after line terminator");
	goto die;
    } else if ( !(in->validator_func(id)) ) { /* Unrecognized identifier. */
#if IGNORE_UNRECOGNIZED_IDENTIFIERS
	;
#else
	interpreter_message("No such identifier");
	goto die;
#endif
    } else if ((errno = in->install_func(id, arg)) != 0) {
	err_ret("%s:%ld: install_func returned %d",
		in->path, in->line_num, errno);
	goto die;
    } else {
	;
    }

    free_not_null(id);
    free_not_null(arg);
    return;

  die:
    free_not_null(id);
    free_not_null(arg);
    abort();
}
