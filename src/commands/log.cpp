/* commands/log.cpp  --  management of log files
   Copyright (C) 2026 Markus Uhlin. All rights reserved.

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

#include <sys/stat.h>

#if HAVE_STD_FS
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <stdexcept>
#include <string>
#include <vector>

#include <inttypes.h>
#include <stdint.h>

#include "../dataClassify.h"
#include "../errHand.h"
#include "../libUtils.h"
#include "../log.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "dcc.h"	/* list_dir() */
#include "i18n.h"
#include "log.h"
#include "theme.h"	/* get_next_line_from_file() */

#if WIN32
#define stat _stat
#endif

class irc_logfile {
public:
	std::string filename;
	std::string fullpath;

	irc_logfile();
	irc_logfile(const std::string &, const std::string &);

	void print(const size_t) const;

private:
	struct stat sb;
};

irc_logfile::irc_logfile()
{
	this->filename.assign("");
	this->fullpath.assign("");

	memset(&this->sb, 0, sizeof(this->sb));
}

irc_logfile::irc_logfile(const std::string &p_filename,
			 const std::string &p_fullpath)
{
	this->filename.assign(p_filename);
	this->fullpath.assign(p_fullpath);

	if (stat(p_fullpath.c_str(), &this->sb) != 0) {
		char strerrbuf[MAXERROR] = { '\0' };

		throw std::runtime_error(xstrerror(errno, strerrbuf,
		    sizeof strerrbuf));
	}
}

const char g_sym_logwin = '=';

static stringarray_t log_cmds = {
	"clear",
	"ls ",
	"ls dir",
	"ls scanned",
	"rm ",
	"scandir",
	"view ",
};

static std::vector<irc_logfile> log_vec;

static void	subcmd_scandir(void);
static void	set_logwin_label(CSTRING, std::string &) NONNULL;

static void
get_time(std::string &str, const time_t *secs)
{
	char		buf[100] = {'\0'};
	struct tm	res = {0};

#if defined(UNIX)
	if (localtime_r(secs, &res) == nullptr) {
		str.assign("unknown");
		return;
	}
#elif defined(WIN32)
	if ((errno = localtime_s(&res, secs)) != 0) {
		str.assign("unknown");
		return;
	}
#endif

	if (strftime(buf, sizeof buf, "%c", &res) == 0) {
		str.assign("unknown");
		return;
	}

	str.assign(&buf[0]);
}

void
irc_logfile::print(const size_t p_no) const
{
	double		size = 0.0;
	char		unit = 'B';
	std::string	str[2];

	dcc::get_file_size(this->sb.st_size, size, unit);

	get_time(str[0], &this->sb.st_atime);
	get_time(str[1], &this->sb.st_mtime);

	printtext_print("sp1", "----- %s%s%s -----",
	    COLOR1, this->filename.c_str(), TXT_NORMAL);
	printtext_print("sp1", "%sno#%s:         " PRINT_SIZE,
	    COLOR2, TXT_NORMAL, p_no);
	printtext_print("sp1", "%ssize%s:        %.1f%c", COLOR2, TXT_NORMAL,
	    size, unit);
	printtext_print("sp1", "%slast access%s: %s", COLOR2, TXT_NORMAL,
	    str[0].c_str());
	printtext_print("sp1", "%slast mod%s:    %s", COLOR2, TXT_NORMAL,
	    str[1].c_str());
	printtext_print("sp1", " ");
}

static void
list_scanned()
{
	if (log_vec.empty()) {
		printtext_print("err", "nothing scanned");
		return;
	}

	size_t	no = 0;
	auto	vec_it = log_vec.begin();

	while (vec_it != log_vec.end()) {
		(*vec_it).print(no);
		++no;
		++vec_it;
	}
}

static void
subcmd_clear()
{
	if (!log_vec.empty())
		log_vec.clear();
}

static void
subcmd_ls(CSTRING p_what)
{
	if (p_what == nullptr || strings_match(p_what, "")) {
		printtext_print("err", "too few arguments");
	} else if (strings_match(p_what, "dir")) {
		list_dir(g_log_dir);
	} else if (strings_match(p_what, "scanned")) {
		list_scanned();
	} else {
		printtext_print("err", "unrecognized argument: %s "
		    "(must be 'dir' or 'scanned')", p_what);
	}
}

static void
subcmd_rm(CSTRING p_no)
{
	UNUSED_PARAM(p_no);
}

static bool
ends_with(const std::string &str, const std::string &ending)
{
	if (ending.size() > str.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

static bool
log_vec_cmp(const irc_logfile &obj1, const irc_logfile &obj2)
{
	const std::string name1(obj1.filename);
	const std::string name2(obj2.filename);

	for (size_t i = 0; i < name1.length() && i < name2.length(); i++) {
		int c1, c2;

		c1 = sw_isupper(name1[i]) ? tolower(name1[i]) : name1[i];
		c2 = sw_isupper(name2[i]) ? tolower(name2[i]) : name2[i];

		if (c1 < c2)
			return true;
		else if (c1 > c2)
			return false;
	}

	return (name1.length() < name2.length() ? true : false);
}

static void
subcmd_scandir(void)
{
#if HAVE_STD_FS
	static const std::string suff1(g_log_filesuffix);
	static const std::string suff2(".log");
	size_t sz;

	if (!log_vec.empty())
		log_vec.clear();

	fs::path path = g_log_dir;
	fs::directory_iterator dir_it(path);

	for (const fs::directory_entry &dir_ent : dir_it) {
		if (!dir_ent.exists() || !dir_ent.is_regular_file())
			continue;

		const std::string filename(dir_ent.path().filename().string());
		const std::string fullpath(dir_ent.path().string());

		if (!ends_with(filename, suff1) && !ends_with(filename, suff2))
			continue;

		irc_logfile file(filename, fullpath);

		log_vec.emplace_back(file);
	}

	sz = static_cast<size_t>(log_vec.size());

	if (sz > 0) {
		std::sort(log_vec.begin(), log_vec.end(), log_vec_cmp);
		printtext_print("success",
		    "A total of " PRINT_SIZE " files found", sz);
	} else
		printtext_print("err", "No files found");
#else	// !HAVE_STD_FS
	printtext_print("err", "operation not supported");
#endif
}

static void
set_logwin_label(CSTRING filename, std::string &strout)
{
	strout.assign(SYM_logwin_str).append(filename).append(SYM_logwin_str);
}

static void
subcmd_view(CSTRING p_no)
{
	FILE		*fp = nullptr;
	STRING		 line = nullptr;
	std::string	 label("");
	uint32_t	 val = 0;

	if (p_no == nullptr || strings_match(p_no, "")) {
		printtext_print("err", "too few arguments");
		return;
	} else if (!is_numeric(p_no)) {
		printtext_print("err", "argument not a number");
		return;
	} else if (log_vec.empty()) {
		printtext_print("err", "vector empty");
		return;
	} else if (xsscanf(p_no, "%" SCNu32, &val) != 1) {
		printtext_print("err", "bad number");
		return;
	}

	if (val > (log_vec.size() - 1)) {
		printtext_print("err", "too high number");
		return;
	}

	const irc_logfile &obj = log_vec[val];

	if ((fp = fopen(obj.fullpath.c_str(), "r")) == nullptr) {
		printtext_print("err", "error opening file");
		return;
	}

	set_logwin_label(obj.filename.c_str(), label);
	errno = spawn_chat_window(label.c_str(), obj.filename.c_str());

	if (errno) {
		fclose(fp);
		printtext_print("err", "error creating window");
		return;
	}

	PRINTTEXT_CONTEXT ptext_ctx;
	printtext_context_init(&ptext_ctx, window_by_label(label.c_str()),
	    TYPE_SPEC_NONE, false);
	if (ptext_ctx.window == nullptr) {
		fclose(fp);
		errno = destroy_chat_window(label.c_str());
		if (errno)
			err_log(errno, "%s: destroy_chat_window", __func__);
		printtext_print("err", "unable to locate the new window "
		    "(shouldn't happen)");
		return;
	} else
		ptext_ctx.window->is_logwin = true;

	while (get_next_line_from_file(fp, &line))
		printtext(&ptext_ctx, "%s", trim(line));

	if (feof(fp))
		printtext_print("success", "wrote %s", obj.filename.c_str());
	else {
		printtext_print("warn", "write incomplete (%s)",
		    obj.filename.c_str());
	}
	fclose(fp);
	free(line);
}

/*
 * usage:
 *     /log <[clear|ls|rm|scandir|view]> [args]
 *     /log clear
 *     /log ls <[dir|scanned]>
 *     /log rm <#>
 *     /log scandir
 *     /log view <#>
 */
void
cmd_log(CSTRING p_data)
{
	CSTRING			arg[2];
	CSTRING			subcmd;
	STRING			dcopy;
	STRING			last = const_cast<STRING>("");
	static chararray_t	cmd = "/log";
	static chararray_t	sep = " ";

	if (strings_match(p_data, "")) {
		printtext_print("err", "%s: too few arguments", cmd);
		return;
	}

	dcopy = sw_strdup(p_data);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: too few arguments", cmd);
		return;
	}

	arg[0] = strtok_r(nullptr, sep, &last);
	arg[1] = strtok_r(nullptr, sep, &last);

	if (arg[1]) {
		printf_and_free(dcopy, "%s: too many arguments", cmd);
		return;
	}

	try {
		if (strings_match(subcmd, "clear"))
			subcmd_clear();
		else if (strings_match(subcmd, "ls"))
			subcmd_ls(arg[0]);
		else if (strings_match(subcmd, "rm"))
			subcmd_rm(arg[0]);
		else if (strings_match(subcmd, "scandir"))
			subcmd_scandir();
		else if (strings_match(subcmd, "view"))
			subcmd_view(arg[0]);
		else {
			printtext_print("err", "%s: invalid subcommand: %s",
			    cmd, subcmd);
		}
	} catch (const std::exception &ex) {
		printtext_print("err", "%s: exception: %s", cmd, ex.what());
	}

	free(dcopy);
}

PTEXTBUF
get_list_of_matching_log_cmds(CSTRING search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	for (size_t i = 0; i < ARRAY_SIZE(log_cmds); i++) {
		CSTRING cmd = log_cmds[i];

		if (!strncmp(search_var, cmd, varlen))
			textBuf_emplace_back(__func__, matches, cmd, 0);
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return nullptr;
	}

	return matches;
}
