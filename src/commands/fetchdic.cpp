#include "common.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "../interpreter.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../strHand.h"

#include "fetchdic.h"
#include "theme.h"

class dictionary {
public:
	dictionary();
	explicit dictionary(const char *);

	std::string lang;
	std::string name;
	std::string date;
	std::string url;

	std::string author;
	std::string license;

	void fetch(void);
};

dictionary::dictionary()
{
	this->lang.assign("");
	this->name.assign("");
	this->date.assign("");
	this->url.assign("");

	this->author.assign("");
	this->license.assign("");
}

dictionary::dictionary(const char *line)
{
	char			*author, *license;
	char			*lang, *name, *date, *url;
	char			*last = const_cast<char *>("");
	char			*line_copy = sw_strdup(line);
	static const char	 sep[] = ";";

	if ((lang = strtok_r(line_copy, sep, &last)) == nullptr ||
	    (name = strtok_r(nullptr, sep, &last)) == nullptr ||
	    (date = strtok_r(nullptr, sep, &last)) == nullptr ||
	    (url = strtok_r(nullptr, sep, &last)) == nullptr ||
	    (author = strtok_r(nullptr, sep, &last)) == nullptr ||
	    (license = strtok_r(nullptr, sep, &last)) == nullptr) {
		free(line_copy);
		throw std::runtime_error("missing tokens");
	}

	this->lang.assign(lang);
	this->name.assign(name);
	this->date.assign(date);
	this->url.assign(url);

	this->author.assign(author);
	this->license.assign(license);

	free(line_copy);
}

void
dictionary::fetch(void)
{
}

static void
dump_db(const std::vector<dictionary> &vec)
{
	UNUSED_PARAM(vec);
}

static void
fetchdic(const char *name, std::vector<dictionary> &vec)
{
	for (dictionary &dic : vec) {
		if (strings_match(name, dic.name.c_str()))
			dic.fetch();
	}
}

static read_result_t
read_db(const char *path, std::vector<dictionary> &vec)
{
	FILE		*fp = nullptr;
	char		*line = nullptr;
	read_result_t	 res = READ_INCOMPLETE;

	if (path == nullptr || (fp = xfopen(path, "r")) == nullptr)
		return FOPEN_FAILED;

	while (get_next_line_from_file(fp, &line)) {
		const char *cp = trim(&line[0]);

		adv_while_isspace(&cp);

		if (strings_match(cp, "") || *cp == '#')
			continue;

		try {
			dictionary dic(cp);
			vec.push_back(dic);
		} catch (...) {
			fclose(fp);
			free(line);
			return PARSE_ERROR;
		}
	}

	res = (feof(fp) ? READ_DB_OK : READ_INCOMPLETE);

	fclose(fp);
	free(line);

	return res;
}

/*
 * usage: /fetchdic [lang]
 */
void
cmd_fetchdic(const char *data)
{
	const std::string	 cmd("/fetchdic");
	std::string		 tmp("");
	std::string		 url("");
	std::vector<dictionary>	 vec;

	tmp.assign(g_tmp_dir).append(SLASH).append("dic_db");
	url.assign(g_swircWebAddr).append("dic_db");

	url_to_file(url.c_str(), tmp.c_str());

	switch (read_db(tmp.c_str(), vec)) {
	case FOPEN_FAILED:
		printtext_print("err", "%s: cannot open database", cmd.c_str());
		return;
	case PARSE_ERROR:
		printtext_print("err", "%s: failed to read database",
		    cmd.c_str());
		return;
	case READ_INCOMPLETE:
		printtext_print("err", "%s: end-of-file indicator not set",
		    cmd.c_str());
		return;
	case READ_DB_OK:
	default:
		break;
	}

	if (strings_match(data, ""))
		dump_db(vec);
	else
		fetchdic(data, vec);
}
