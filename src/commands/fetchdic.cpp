/* fetchdic.cpp
   Copyright (C) 2023-2025 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "../errHand.h"
#include "../filePred.h"
#include "../interpreter.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "fetchdic.h"
#include "theme.h"

const char	 g_aff_suffix[AFF_SUFFIX_LEN] = ".aff";
const char	 g_dic_suffix[DIC_SUFFIX_LEN] = ".dic";

struct dictionary {
	dictionary();
	explicit dictionary(const char *);

	std::string	 lang;
	std::string	 name;
	std::string	 date;
	std::string	 url;

	std::string	 author;
	std::string	 license;

	void	 fetch(void) const;
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
	char			*last = const_cast<char *>("");
	char			*line_copy = sw_strdup(line);
	const char		*token[6] = { nullptr };
	static const char	 sep[] = "|";

	token[0] = strtok_r(line_copy, sep, &last);    /* lang    */
	token[1] = strtok_r(nullptr, sep, &last);      /* name    */
	token[2] = strtok_r(nullptr, sep, &last);      /* date    */
	token[3] = strtok_r(nullptr, sep, &last);      /* url     */
	token[4] = strtok_r(nullptr, sep, &last);      /* author  */
	token[5] = strtok_r(nullptr, sep, &last);      /* license */

	if (token[0] == nullptr || token[1] == nullptr || token[2] == nullptr ||
	    token[3] == nullptr || token[4] == nullptr || token[5] == nullptr) {
		free(line_copy);
		throw std::runtime_error("missing tokens");
	}

	try {
		this->lang.assign(token[0]);
		this->name.assign(token[1]);
		this->date.assign(token[2]);
		this->url.assign(token[3]);

		this->author.assign(token[4]);
		this->license.assign(token[5]);
	} catch (...) {
		/* null */;
	}

	free(line_copy);
}

void
dictionary::fetch(void) const
{
	std::string	 aff_file(g_home_dir);
	std::string	 aff_url(this->url);
	std::string	 dic_file(g_home_dir);
	std::string	 dic_url(this->url);

	aff_file.append(SLASH).append(this->name).append(g_aff_suffix);
	dic_file.append(SLASH).append(this->name).append(g_dic_suffix);

	aff_url.append(this->name).append(g_aff_suffix);
	dic_url.append(this->name).append(g_dic_suffix);

	printtext_print(nullptr, " - Fetching...");
	url_to_file(aff_url.c_str(), aff_file.c_str());

	if (is_regular_file(aff_file.c_str()))
		printtext_print("success", "Fetched %s", aff_file.c_str());
	else
		printtext_print("err", "Error %s", g_aff_suffix);

	printtext_print(nullptr, " - Fetching...");
	url_to_file(dic_url.c_str(), dic_file.c_str());

	if (is_regular_file(dic_file.c_str()))
		printtext_print("success", "Fetched %s", dic_file.c_str());
	else
		printtext_print("err", "Error %s", g_dic_suffix);

	printtext_print(nullptr, " - Completed");
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
	for (const dictionary &dic : vec) {
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

#if defined(__cplusplus) && __cplusplus >= 201103L
			vec.emplace_back(dic);
#else
			vec.push_back(dic);
#endif
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
		/* No statements here ATM. */
		break;
	}

	if (strings_match(data, ""))
		dump_db(vec);
	else
		fetchdic(data, vec);

	if (remove(tmp.c_str()) != 0) {
		err_log(errno, "%s: failed to remove: %s", __func__,
		    tmp.c_str());
	}
}
