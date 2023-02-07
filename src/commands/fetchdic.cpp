#include "common.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "../filePred.h"
#include "../interpreter.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../spell.h"
#include "../strHand.h"
#include "../theme.h"

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
	static const char	 sep[] = "|";

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
	std::string	 aff_file(g_home_dir);
	std::string	 aff_url(this->url);
	std::string	 dic_file(g_home_dir);
	std::string	 dic_url(this->url);

	aff_file.append(SLASH).append(this->name).append(g_aff_suffix);
	dic_file.append(SLASH).append(this->name).append(g_dic_suffix);

	aff_url.append(this->name).append(g_aff_suffix);
	dic_url.append(this->name).append(g_dic_suffix);

	url_to_file(aff_url.c_str(), aff_file.c_str());
	url_to_file(dic_url.c_str(), dic_file.c_str());

	if (is_regular_file(aff_file.c_str()))
		printtext_print("success", "Fetched %s", aff_file.c_str());
	if (is_regular_file(dic_file.c_str()))
		printtext_print("success", "Fetched %s", dic_file.c_str());
}

static void
dump_db(const std::vector<dictionary> &vec)
{
	for (const dictionary &dic : vec) {
		printtext_print("sp1", "----- %s%s%s -----",
		    COLOR1, dic.lang.c_str(), TXT_NORMAL);
		printtext_print("sp1", "name:     %s", dic.name.c_str());
		printtext_print("sp1", "date:     %s", dic.date.c_str());
		printtext_print("sp1", "url:      %s", dic.url.c_str());
		printtext_print("sp1", "author:   %s", dic.author.c_str());
		printtext_print("sp1", "license:  %s", dic.license.c_str());
		printtext_print("sp1", " ");
	}
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
 * usage: /fetchdic [name]
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
