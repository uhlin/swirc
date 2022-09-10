/* Create the home directory and read its configuration files
   Copyright (C) 2012-2022 Markus Uhlin. All rights reserved.

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

#if defined(UNIX)
#include <sys/stat.h>
#include <sys/types.h>
#elif defined(WIN32)
#include <direct.h> /* _mkdir() */
#endif

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include "assertAPI.h"
#include "config.h"
#include "crypt.h"
#include "dataClassify.h"
#include "errHand.h"
#include "filePred.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "network.h"
#include "ossl-scripts.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "theme.h"

#include "commands/sasl.h"

char	*g_user = NULL;

char	*g_home_dir = NULL;
char	*g_tmp_dir = NULL;
char	*g_log_dir = NULL;

const char	g_config_filesuffix[] = ".conf";
const char	g_theme_filesuffix[] = ".the";

char	*g_config_file = NULL;
char	*g_theme_file = NULL;

static void
create_openssl_scripts(void)
{
	create_root_ca_script();
	create_server_ca_script();
	create_server_cert_script();
	create_client_cert_script();
	create_dhparams_script();
}

static void
make_requested_dir(const char *path)
{
	if (is_directory(path)) {
		/*
		 * Already exists!
		 */
		return;
	} else if (file_exists(path)) {
		err_quit("%s exists. However; it isn't a directory.", path);
	} else {
#if defined(UNIX)
		if (mkdir(path, S_IRWXU) != 0)
			err_sys("mkdir error");
#elif defined(WIN32)
		if (_mkdir(path) != 0)
			err_sys("_mkdir error");
#endif
	}
}

static void
modify_setting(const char *setting_name, const char *new_value)
{
	(void) config_item_undef(setting_name);
	(void) config_item_install(setting_name, new_value);
}

static bool
passwd_ok_check(const char *str)
{
	for (const char *cp = str; *cp != '\0'; cp++) {
		if (strchr(g_sasl_pass_allowed_chars, *cp) == NULL)
			return false;
	}
	return true;
}

static void
prompt_for_decryption(const char *str)
{
	char *dc_out = NULL;

	puts("Please decrypt your SASL password...");
	puts("(1 attempt)");

	while (true) {
		bool	 fgets_error;
		char	*value;
		char	 pass[400] = { '\0' };

		printf("Decryption pass (will echo): ");
		fflush(stdout);

		fgets_error = fgets(pass, sizeof pass, stdin) == NULL;

		if (fgets_error) {
			continue;
		} else if (strchr(pass, '\n') == NULL) {
			int	c;

			puts("Input too big");

			while (c = getchar(), c != '\n' && c != EOF)
				/* discard */;

			continue;
		}

		pass[strcspn(pass, "\n")] = '\0';

		if (strings_match(pass, "")) {
			continue;
		} else if ((dc_out = crypt_decrypt_str(str, (cryptstr_t)
		    addrof(pass[0]), true)) == NULL) {
			puts("Decryption failed");
			break;
		} else if (!passwd_ok_check(dc_out)) {
			puts("Wrong pass");
			break;
		}

		puts("Pass seems reasonable");
		value = strdup_printf("%c%s", g_decrypted_pass_sym, dc_out);
		modify_setting("sasl_password", value);
		crypt_freezero(value, strlen(value));
		crypt_freezero(dc_out, strlen(dc_out));
		dc_out = NULL;

		break;
	}

	free(dc_out);

	puts("Press <RETURN>");
	(void) getchar();
}

static void
save_cmdline_opts(const char *path)
{
	if (g_cmdline_opts->nickname)
		modify_setting("nickname", g_cmdline_opts->nickname);
	if (g_cmdline_opts->username)
		modify_setting("username", g_cmdline_opts->username);
	if (g_cmdline_opts->rl_name)
		modify_setting("real_name", g_cmdline_opts->rl_name);

	config_do_save(path, "w");
}

void
nestHome_init(void)
{
#define EXPLICIT_CONFIG g_cmdline_opts->config_file
	char *hp = path_to_home() ? sw_strdup(path_to_home()) : NULL;
	const char *cp;

	if (isNull(hp))
		err_quit("Can't resolve homepath!");

	g_user = sw_strdup(getuser());

#if defined(UNIX)
	g_home_dir = strdup_printf("%s/.swirc", hp);
	g_tmp_dir = strdup_printf("%s/.swirc/tmp", hp);
	g_log_dir = strdup_printf("%s/.swirc/log", hp);
	g_config_file = strdup_printf("%s/.swirc/swirc%s", hp,
	    g_config_filesuffix);
#elif defined(WIN32)
	g_home_dir = strdup_printf("%s\\swirc", hp);
	g_tmp_dir = strdup_printf("%s\\swirc\\tmp", hp);
	g_log_dir = strdup_printf("%s\\swirc\\log", hp);
	g_config_file = strdup_printf("%s\\swirc\\swirc%s", hp,
	    g_config_filesuffix);
#endif

	make_requested_dir(g_home_dir);
	make_requested_dir(g_tmp_dir);
	make_requested_dir(g_log_dir);

	config_init();
	theme_init();

	if (g_explicit_config_file) {
		if (file_exists(EXPLICIT_CONFIG) &&
		    !is_regular_file(EXPLICIT_CONFIG)) {
			err_quit("%s exists  --  but isn't a regular file.",
			    EXPLICIT_CONFIG);
		} else if (!file_exists(EXPLICIT_CONFIG)) {
			err_quit("%s no such file or directory. Exiting...",
			    EXPLICIT_CONFIG);
		} else {
			config_readit(EXPLICIT_CONFIG, "r");
		}
	} else if (!g_explicit_config_file) {
		if (file_exists(g_config_file) &&
		    !is_regular_file(g_config_file)) {
			err_quit("%s exists  --  but isn't a regular file.",
			    g_config_file);
		} else if (!file_exists(g_config_file)) {
			config_create(g_config_file, "w+");
			config_readit(g_config_file, "r");
		} else {
			config_readit(g_config_file, "r");
		}

		if (g_cmdline_opts->nickname || g_cmdline_opts->username ||
		    g_cmdline_opts->rl_name)
			save_cmdline_opts(g_config_file);
	} else {
		sw_assert_not_reached();
	}

	if (sasl_is_enabled() && get_sasl_passwd_type() ==
	    g_encrypted_pass_sym) {
		cp = Config("sasl_password");
		if (!strings_match(cp, ""))
			prompt_for_decryption(cp);
	}

#if defined(UNIX)
	g_theme_file = strdup_printf("%s/.swirc/%s%s", hp, Config("theme"),
	    g_theme_filesuffix);
#elif defined(WIN32)
	g_theme_file = strdup_printf("%s\\swirc\\%s%s", hp, Config("theme"),
	    g_theme_filesuffix);
#endif

	if (isEmpty(Config("theme"))) {
		err_quit("Item theme in user config file holds no data. "
		    "Error.");
	} else if (file_exists(g_theme_file) &&
	    !is_regular_file(g_theme_file)) {
		err_quit("%s exists  --  but isn't a regular file.",
		    g_theme_file);
	} else if (!file_exists(g_theme_file) &&
	    strncmp(Config("theme"), "default", 8) == 0) {
		theme_create(g_theme_file, "w+");
		theme_readit(g_theme_file, "r");
	} else if (!file_exists(g_theme_file) &&
	    strncmp(Config("theme"), "default", 8) != 0) {
		err_quit("%s no such file or directory. Exiting...",
		    g_theme_file);
	} else {
		theme_readit(g_theme_file, "r");
	}

	free(hp);
	create_openssl_scripts();
}

void
nestHome_deinit(void)
{
	free_and_null(&g_user);

	free_and_null(&g_home_dir);
	free_and_null(&g_tmp_dir);
	free_and_null(&g_log_dir);

	free_and_null(&g_config_file);
	free_and_null(&g_theme_file);

	config_deinit();
	theme_deinit();
}

const char *
path_to_home(void)
{
	char *var_data;
	const char var[] =
#if defined(UNIX)
	    "HOME";
#elif defined(WIN32)
	    "APPDATA";
#endif
	static char buf[PATH_MAX] = { '\0' };

/*
 * getenv() is safe in this context
 */
#if WIN32
#pragma warning(disable: 4996)
#endif
	if ((var_data = getenv(var)) == NULL ||
	    sw_strcpy(buf, var_data, ARRAY_SIZE(buf)) != 0 ||
	    !is_directory(buf))
		return (NULL);
/*
 * Reset warning behavior to its default value
 */
#if WIN32
#pragma warning(default: 4996)
#endif

	return (&buf[0]);
}
