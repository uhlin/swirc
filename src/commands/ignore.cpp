#include "common.h"

#include <climits>
#include <regex>
#include <stdexcept>
#include <string>

#include "../libUtils.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ignore.h"

#define MAXIGNORES 30

class ignore {
    std::string str;
    std::regex regex;

public:
    explicit ignore(const char *);

    std::string &get_str(void) {
	return this->str;
    }

    std::regex &get_regex(void) {
	return this->regex;
    }
};

ignore::ignore(const char *_str)
{
    this->str.assign(_str);
    this->regex.assign(_str);
}

static const size_t regex_maxlen = 60;
static std::vector<ignore> ignore_list;

static void
print_ignore_list()
{
    PRINTTEXT_CONTEXT ctx;
    int no;
    std::vector<ignore>::iterator it;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);
    printtext(&ctx, "List of ignores:");

    no = 0;
    it = ignore_list.begin();

    while (it != ignore_list.end()) {
	printtext(&ctx, "%d: %s", no, it->get_str().c_str());
	++ no;
	++ it;
    }
}

/*
 * usage: /ignore [regex]
 */
void
cmd_ignore(const char *data)
{
    PRINTTEXT_CONTEXT ctx;
    char *err_reason = NULL;

    if (strings_match(data, "")) {
	print_ignore_list();
	return;
    } else if (!is_valid_regex(data, &err_reason)) {
	print_and_free(err_reason, err_reason);
	return;
    }

    ignore object(data);
    ignore_list.push_back(object);

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);
    printtext(&ctx, "Added \"%s\" to ignore list.", object.get_str().c_str());
}

/*
 * usage: /unignore [#]
 */
void
cmd_unignore(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;

	if (strings_match(data, "")) {
		print_ignore_list();
		return;
	}

	try {
		char		*ep = const_cast<char *>("");
		char		*regex;
		long int	 no;

		if (ignore_list.empty())
			throw std::runtime_error("ignore list empty");

		errno = 0;
		no = strtol(data, &ep, 10);

		if (data[0] == '\0' || *ep != '\0')
			throw std::runtime_error("not a number");
		else if (errno == ERANGE && (no == LONG_MAX || no == LONG_MIN))
			throw std::runtime_error("out of range");
		else if (no < 0 ||
		    static_cast<size_t>(no) > ignore_list.size() ||
		    no > MAXIGNORES)
			throw std::runtime_error("out of range");

		regex = sw_strdup(ignore_list.at(no).get_str().c_str());
		ignore_list.erase(ignore_list.begin() + no);
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_SUCCESS, true);
		printtext(&ctx, "Deleted \"%s\" from ignore list.", regex);
		free(regex);
		print_ignore_list();
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s", e.what());
	}
}

bool
is_valid_regex(const char *str, char **err_reason)
{
	if (strings_match(str, "")) {
		*err_reason = sw_strdup("no regex");
		return false;
	} else if (strlen(str) > regex_maxlen) {
		*err_reason = sw_strdup("regex too long");
		return false;
	}

	try {
		std::regex	regex(str);
	} catch (const std::regex_error& e) {
		*err_reason = sw_strdup(e.what());
		return false;
	}

	*err_reason = NULL;
	return true;
}
