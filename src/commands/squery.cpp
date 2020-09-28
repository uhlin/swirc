#include "common.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "squery.h"

/*
 * usage: /squery <servicename> <text>
 */
void
cmd_squery(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/squery: missing arguments", NULL);
	return;
    }

    char *dcopy = sw_strdup(data);
    (void) strFeed(dcopy, 1);
    std::istringstream input(dcopy);
    free_and_null(&dcopy);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(input, token))
	tokens.push_back(token);

    try {
	if (tokens.size() != 2)
	    throw std::runtime_error("missing arguments");

	const char *servicename = tokens.at(0).c_str();
	const char *text = tokens.at(1).c_str();

	if (net_send("SQUERY %s :%s", servicename, text) < 0)
	    throw std::runtime_error("cannot send");
    } catch (std::runtime_error &e) {
	std::string s("/squery: ");
	s.append(e.what());
	print_and_free(s.c_str(), NULL);
    }
}
