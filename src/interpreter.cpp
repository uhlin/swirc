/* Copyright (c) 2012-2022 Markus Uhlin <markus.uhlin@bredband.net>
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
#include "strHand.h"

#include <iostream>
#include <stdexcept>

static const size_t	identifier_maxSize = 50;
static const size_t	argument_maxSize = 480;

/**
 * Copy identifier
 */
static char *
copy_identifier(const char *&id)
{
	size_t	 count = identifier_maxSize;
	char	*dest_buf = new char[count + 1];
	char	*dest = addrof(dest_buf[0]);

	while ((sw_isalnum(*id) || *id == '_') && count > 1) {
		*dest++ = *id++, count--;
	}

	*dest = '\0';

	if (count == 1)
		err_exit(EOVERFLOW, "In copy_identifier: fatal: "
		    "string was truncated!");
	return dest_buf;
}

/**
 * Copy argument
 */
/*lint -sem(copy_argument, r_null) */
static char *
copy_argument(const char *&arg)
{
	bool	 inside_arg = true;
	size_t	 count = argument_maxSize;
	char	*dest_buf = new char[count + 1];
	char	*dest = addrof(dest_buf[0]);

	while (*arg && count > 1) {
		if (*arg == '\"') {
			inside_arg = false;
			arg++;
			break;
		}

		*dest++ = *arg++, count--;
	}

	*dest = '\0';

	if (inside_arg && count == 1)
		err_exit(EOVERFLOW, "In copy_argument: fatal: "
		    "string was truncated!");
	if (inside_arg) {
		delete[] dest_buf;
		return NULL;
	}
	return dest_buf;
}

static void
clean_up(char *id, char *arg)
{
	if (id)
		delete[] id;
	if (arg)
		delete[] arg;
}

/**
 * Interpreter
 *
 * @param in Context structure
 * @return Void
 *
 * An interpreter for configuration files. The context structure
 * contains the data to be passed to the interpreter.
 */
void
Interpreter(const struct Interpreter_in *in)
{
	char	*id = NULL;
	char	*arg = NULL;

	try {
		if (in == NULL)
			throw std::runtime_error("null input");

		const char *cp = addrof(in->line[0]);

		if (!sw_isalnum(*cp) && *cp != '_')
			throw std::runtime_error("unexpected leading "
			    "character");
		id = copy_identifier(cp);
		adv_while_isspace(&cp);
		if (*cp++ != '=') {
			throw std::runtime_error("expected assignment "
			    "operator");
		}

		adv_while_isspace(&cp);
		if (*cp++ != '\"')
			throw std::runtime_error("expected string");
		else if ((arg = copy_argument(cp)) == NULL)
			throw std::runtime_error("unterminated argument");

		adv_while_isspace(&cp);
		if (*cp++ != ';')
			throw std::runtime_error("no line terminator!");

		adv_while_isspace(&cp);
		if (*cp && *cp != '#') {
			throw std::runtime_error("implicit data after "
			    "line terminator!");
		} else if (!(in->validator_func(id))) {
#if IGNORE_UNRECOGNIZED_IDENTIFIERS
			/* ignore */;
#else
			throw std::runtime_error("no such identifier");
#endif
		} else if ((errno = in->install_func(id, arg)) != 0) {
			throw std::runtime_error("install error");
		}
	} catch (const std::runtime_error& e) {
		std::cerr << '\t' << in->line << '\n';

		if (strings_match(e.what(), "install error")) {
			err_ret("%s:%ld: error: install_func returned %d",
			    in->path, in->line_num, errno);
		} else {
			err_msg("%s:%ld: error: %s", in->path, in->line_num,
			    e.what());
		}

		clean_up(id, arg);
		abort();
	}

	clean_up(id, arg);
}

void
Interpreter_processAllLines(FILE *fp, const char *path, Interpreter_vFunc func1,
    Interpreter_instFunc func2)
{
	char buf[MAXLINE] = { '\0' };
	long int line_num = 0;

	while (fgets(buf, sizeof buf, fp) != NULL) {
		char *line;
		const char *cp;
		struct Interpreter_in in;

		cp = &buf[0];
		adv_while_isspace(&cp);
		if (strings_match(cp, "") || *cp == '#') {
			line_num++;
			continue;
		}

		line = trim(sw_strdup(cp));

		in.path			= const_cast<char *>(path);
		in.line			= line;
		in.line_num		= ++line_num;
		in.validator_func	= func1;
		in.install_func		= func2;
		Interpreter(&in);

		free(line);
	}
}
