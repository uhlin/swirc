/* commands/sasl.cpp
   Copyright (C) 2017-2022 Markus Uhlin. All rights reserved.

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

#include "base64.h"
#include "common.h"

#include <openssl/pem.h>

#include <climits>
#include <stdexcept>
#include <string>

#include "../irc.h"
#include "../events/cap.h"

#include "../config.h"
#include "../crypt.h"
#include "../errHand.h"
#include "../filePred.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"

#include "sasl.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

const char	g_decrypted_pass_sym = '%';
const char	g_encrypted_pass_sym = '#';
const char	g_unencrypted_pass_sym = '?';

const char g_sasl_pass_allowed_chars[] =
    "!#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~";

static const size_t	password_min = 8;
static const size_t	password_max = 200;

static stringarray_t sasl_cmds = {
	"keygen",
	"pubkey",
	"mechanism ",
	"mechanism ecdsa-nist256p-challenge",
	"mechanism plain",
	"mechanism scram-sha-256",
	"username ",
	"password ",
	"passwd_s ",
	"set ",
	"set on",
	"set off",
};

/*lint -sem(get_filepath, r_null) */
static char *
get_filepath(const bool is_public)
{
	static char	path[PATH_MAX] = { '\0' };

	if (sw_strcpy(path, g_home_dir, sizeof path) != 0)
		return NULL;
	else if (sw_strcat(path, SLASH, sizeof path) != 0)
		return NULL;
	else if (sw_strcat(path, (is_public ? "ec_key.pub" : "ec_key"),
	    sizeof path) != 0)
		return NULL;
	return (&path[0]);
}

static void
output_message(const bool is_error, const char *message)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, (is_error ?
	    TYPE_SPEC1_FAILURE : TYPE_SPEC1_SUCCESS), true);
	printtext(&ctx, "%s", message);
}

static void
sasl_keygen(const bool force)
{
	EC_KEY	*key = NULL;
	FILE	*fp = NULL;

	try {
		char	*path;

		if ((path = get_filepath(false)) == NULL) {
			throw std::runtime_error("unable to get file path");
		} else if (file_exists(path) && !force) {
			throw std::runtime_error("file exists! add --force to "
			    "overwrite it");
		} else if ((key = EC_KEY_new_by_curve_name
		    (NID_X9_62_prime256v1)) == NULL) {
			throw std::runtime_error("unable to construct ec key");
		} else {
			EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
		}

		if (!EC_KEY_generate_key(key))
			throw std::runtime_error("key generation failed");
		else if ((fp = xfopen(path, "w")) == NULL)
			throw std::runtime_error("unable to open file");
		else if (!PEM_write_ECPrivateKey(fp, key, NULL, NULL, 0, NULL,
		    NULL))
			throw std::runtime_error("unable to write private key");

		output_message(false, "key generation successful");
	} catch (const std::runtime_error &e) {
		output_message(true, e.what());
	} catch (...) {
		output_message(true, "unknown exception was thrown!");
	}

	if (key)
		EC_KEY_free(key);
	if (fp)
		(void) fclose(fp);
}

static int
public_key_length(EC_KEY *key)
{
	if (key == NULL)
		return -1;
	return (i2o_ECPublicKey(key, NULL));
}

/*lint -sem(public_key_blob, r_null) */
static unsigned char *
public_key_blob(EC_KEY *key)
{
	int		 length;
	unsigned char	*out, *out_p;

	if (key == NULL || (length = public_key_length(key)) <= 0)
		return NULL;

	out = static_cast<unsigned char *>(xmalloc(length));
	out[length - 1] = '\0';
	out_p = &out[0];

	if (i2o_ECPublicKey(key, &out_p) < 0) {
		free(out);
		return NULL;
	}

	return out;
}

static void
sasl_pubkey(void)
{
	EC_KEY		*key = NULL;
	FILE		*fp = NULL;
	char		*encoded_blob = NULL;
	unsigned char	*blob = NULL;

	try {
		char	*msg, *path;
		int	 encode_len,
			 encode_ret;
		int	 length = -1;

		if ((key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) ==
		    NULL)
			throw std::runtime_error("unable to construct ec key");
		else
			EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);

		if ((path = get_filepath(false)) == NULL) {
			throw std::runtime_error("unable to get file path");
		} else if (!file_exists(path)) {
			throw std::runtime_error("unable to find private key "
			    "(doesn't exist  --  use keygen)");
		} else if ((fp = xfopen(path, "r")) == NULL) {
			throw std::runtime_error("unable to open private key");
		} else if (PEM_read_ECPrivateKey(fp, &key, NULL, NULL) ==
		    NULL) {
			throw std::runtime_error("unable to read private key");
		} else if (!EC_KEY_check_key(key)) {
			throw std::runtime_error("bogus key");
		} else if ((length = public_key_length(key)) <= 0) {
			throw std::runtime_error("public key length error");
		} else if ((blob = public_key_blob(key)) == NULL) {
			throw std::runtime_error("public key blob error");
		} else if ((encode_len = crypt_get_base64_encode_length(length))
		    <= 0) {
			throw std::runtime_error("encode length error");
		}

		encoded_blob = static_cast<char *>(xmalloc(encode_len));
		encoded_blob[encode_len - 1] = '\0';

		if ((encode_ret = b64_encode(blob, static_cast<size_t>(length),
		    encoded_blob, static_cast<size_t>(encode_len))) == -1)
			throw std::runtime_error("encoding error");
		UNUSED_VAR(encode_ret);

		msg = strdup_printf("public key is: %s", encoded_blob);
		output_message(false, msg);
		free(msg);
	} catch (const std::runtime_error &e) {
		output_message(true, e.what());
	} catch (...) {
		output_message(true, "unknown exception was thrown!");
	}

	if (key)
		EC_KEY_free(key);
	if (fp != NULL)
		(void) fclose(fp);
	free(encoded_blob);
	free(blob);
}

static bool
modify_setting(const char *setting_name, const char *new_value)
{
	if (config_item_undef(setting_name) != 0 ||
	    config_item_install(setting_name, new_value) != 0)
		return false;
	return true;
}

static void
save_to_config(void)
{
	char	 path[PATH_MAX] = { '\0' };
	size_t	 len = 0;

	if (g_home_dir == NULL)
		return;

	len += strlen(g_home_dir);
	len += strlen(SLASH);
	len += strlen("swirc");
	len += strlen(g_config_filesuffix);

	if (len >= sizeof path) {
		err_log(ENAMETOOLONG, "save_to_config");
		return;
	}

	(void) sw_strcpy(path, g_home_dir, sizeof path);
	(void) sw_strcat(path, SLASH, sizeof path);
	(void) sw_strcat(path, "swirc", sizeof path);
	(void) sw_strcat(path, g_config_filesuffix, sizeof path);

	config_do_save(path, "w");
}

static void
set_mechanism(char *mechanism)
{
	(void) strToUpper(mechanism);

	if (!is_sasl_mechanism_supported(mechanism)) {
		output_message(true, "sasl mechanism unknown");
		return;
	} else if (!modify_setting("sasl_mechanism", mechanism)) {
		output_message(true, "set mechanism failed");
		return;
	} else {
		output_message(false, "ok");
		save_to_config();
	}
}

static void
set_username(const char *username)
{
	if (!modify_setting("sasl_username", username)) {
		output_message(true, "set username failed");
		return;
	} else {
		output_message(false, "set username ok");
		save_to_config();
	}
}

static void
set_password(const char *password)
{
	std::string str("");

	str.push_back(g_unencrypted_pass_sym);
	str.append(password);

	if (!modify_setting("sasl_password", str.c_str())) {
		output_message(true, "set password failed");
		return;
	} else {
		output_message(false, "set password ok");
		save_to_config();
	}
}

static bool
passwd_ok_check(const char *str, std::string &err_reason)
{
	if (str == NULL) {
		err_reason.assign("is null");
		return false;
	} else if (strings_match(str, "")) {
		err_reason.assign("is empty");
		return false;
	}

	for (const char *cp = str; *cp != '\0'; cp++) {
		if (strchr(g_sasl_pass_allowed_chars, *cp) == NULL) {
			err_reason.assign("contains forbidden chars");
			return false;
		}
	}

	if (strlen(str) < password_min) {
		err_reason.assign("is too short");
		return false;
	} else if (strlen(str) > password_max) {
		err_reason.assign("is too long");
		return false;
	}

	err_reason.assign("");
	return true;
}

static void
set_passwd_s(const char *data)
{
	bool	 error = false;
	char	*cout = NULL;
	char	*data_copy = NULL;
	char	*val = NULL;
	size_t	 data_len = 0;

	try {
		char		*last = const_cast<char *>("");
		char		*sasl_pass, *encryption_pass;
		cryptstr_t	 str1, str2;
		std::string	 err_reason("");
		std::string	 msg("");

		if (data == NULL || strings_match(data, ""))
			throw std::runtime_error("no data");

		data_copy = sw_strdup(data);
		data_len = strlen(data);

		if ((sasl_pass = strtok_r(data_copy, " ", &last)) == NULL ||
		    (encryption_pass = strtok_r(NULL, " ", &last)) == NULL) {
			throw std::runtime_error("too few args");
		} else if (strtok_r(NULL, " ", &last) != NULL) {
			throw std::runtime_error("too many args");
		} else if (!passwd_ok_check(sasl_pass, err_reason)) {
			msg.assign("SASL pass: ").append(err_reason);
			throw std::runtime_error(msg.c_str());
		} else if (!passwd_ok_check(encryption_pass, err_reason)) {
			msg.assign("Encryption pass: ").append(err_reason);
			throw std::runtime_error(msg.c_str());
		}

		str1 = reinterpret_cast<cryptstr_t>(sasl_pass);
		str2 = reinterpret_cast<cryptstr_t>(encryption_pass);

		if ((cout = crypt_encrypt_str(str1, str2, true)) == NULL)
			throw std::runtime_error("encryption failed");

		val = strdup_printf("%c%s", g_encrypted_pass_sym, cout);
		free_and_null(&cout);

		if (!modify_setting("sasl_password", val))
			throw std::runtime_error("unable to modify setting");
		save_to_config();
	} catch (const std::runtime_error &e) {
		error = true;
		printtext_print("err", "%s", e.what());
	} catch (...) {
		error = true;
		printtext_print("err", "%s", "unknown exception was thrown!");
	}

	if (!error)
		printtext_print("success", "sasl_password = %s",
		    Config("sasl_password"));
	crypt_freezero(data_copy, data_len);
	free(cout);
	free(val);
}

static void
set_state(char *state)
{
	(void) strToLower(state);

	if (!strings_match(state, "on") && !strings_match(state, "off")) {
		output_message(true, "what? on or off?");
		return;
	} else if (!modify_setting("sasl", state)) {
		output_message(true, "set sasl on/off failed");
		return;
	} else {
		char	*msg;

		msg = strdup_printf("SASL authentication is now %s",
		    (strings_match(state, "on") ? "ON" : "OFF"));
		output_message(false, msg);
		free(msg);
		save_to_config();
	}
}

/* usage: /sasl <operation> [...]

   keygen [--force]
   pubkey
   mechanism [ecdsa-nist256p-challenge | plain | scram-sha-256]
   username <name>
   password <pass>
   passwd_s <sasl pass> <encryption pass>
   set [on | off] */
void
cmd_sasl(const char *data)
{
	char mechanism[31] = { '\0' };
	char username[101] = { '\0' };
	char password[301] = { '\0' };
	char state[11] = { '\0' };

/*
 * sscanf() is safe in this context
 */
#if WIN32
#pragma warning(disable: 4996)
#endif

	if (strings_match(data, "keygen") || strings_match(data, "KEYGEN"))
		sasl_keygen(false);
	else if (strings_match(data, "keygen --force") ||
		 strings_match(data, "KEYGEN --force"))
		sasl_keygen(true);
	else if (strings_match(data, "pubkey") || strings_match(data, "PUBKEY"))
		sasl_pubkey();
	else if (sscanf(data, "mechanism %30s", mechanism) == 1)
		set_mechanism(mechanism);
	else if (sscanf(data, "username %100s", username) == 1)
		set_username(username);
	else if (sscanf(data, "password %300s", password) == 1)
		set_password(password);
	else if (!strncmp(data, "passwd_s ", 9))
		set_passwd_s(addrof(data[9]));
	else if (sscanf(data, "set %10s", state) == 1)
		set_state(state);
	else
		output_message(true, "bogus operation");

/*
 * Reset warning behavior to its default value
 */
#if WIN32
#pragma warning(default: 4996)
#endif
}

static bool
sign_decoded_data(EC_KEY *key, const uint8_t *data, int datalen, uint8_t **sig,
    unsigned int *siglen)
{
	unsigned int	len = 0;

	if (key == NULL ||
	    (len = static_cast<unsigned int>(ECDSA_size(key))) == 0) {
		*sig = NULL;
		*siglen = 0;
		return false;
	}

	*sig = static_cast<uint8_t *>(xmalloc(len));
	(*sig)[len - 1] = '\0';

	if (!ECDSA_sign(0, data, datalen, *sig, &len, key)) {
		free(*sig);
		*sig = NULL;
		*siglen = 0;
		return false;
	}

	*siglen = len;
	return true;
}

static bool
got_hits(const char *search_var)
{
	for (size_t i = 0; i < ARRAY_SIZE(sasl_cmds); i++) {
		if (!strncmp(search_var, sasl_cmds[i], strlen(search_var)))
			return true;
	}

	return false;
}

PTEXTBUF
get_list_of_matching_sasl_cmds(const char *search_var)
{
#define MSG "get_list_of_matching_sasl_cmds: textBuf_ins_next"
	PTEXTBUF	matches;

	if (!got_hits(search_var))
		return NULL;

	matches = textBuf_new();

	for (size_t i = 0; i < ARRAY_SIZE(sasl_cmds); i++) {
		const char	*cmd = sasl_cmds[i];

		if (!strncmp(search_var, cmd, strlen(search_var))) {
			if (textBuf_size(matches) == 0) {
				if ((errno = textBuf_ins_next(matches, NULL,
				    cmd, -1)) != 0)
					err_sys(MSG);
			} else {
				if ((errno = textBuf_ins_next(matches,
				    textBuf_tail(matches), cmd, -1)) != 0)
					err_sys(MSG);
			}
		}
	}

	return matches;
}


static void
clean_up(EC_KEY *key, FILE *fp, char *out, uint8_t *decoded_chl, uint8_t *sig)
{
	if (key)
		EC_KEY_free(key);
	if (fp != NULL && fclose(fp) != 0)
		err_log(errno, "solve_ecdsa_nist256p_challenge: fclose");
	free(out);
	free(decoded_chl);
	free(sig);
}

char *
solve_ecdsa_nist256p_challenge(const char *challenge, char **err_reason)
{
	EC_KEY		*key = NULL;
	FILE		*fp = NULL;
	char		*out = NULL;
	uint8_t		*decoded_chl = NULL;
	uint8_t		*sig = NULL;

	try {
		char		*path = NULL;
		int		 decode_len,
				 encode_len;
		int		 decode_ret = -1;
		unsigned int	 siglen = 0;

		if ((key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) ==
		    NULL)
			throw std::runtime_error("unable to construct ec key");
		else
			EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);

		if ((path = get_filepath(false)) == NULL)
			throw std::runtime_error("unable to get file path");
		else if (!file_exists(path))
			throw std::runtime_error("unable to find private key");
		else if ((fp = xfopen(path, "r")) == NULL)
			throw std::runtime_error("unable to open private key");
		else if (PEM_read_ECPrivateKey(fp, &key, NULL, NULL) == NULL)
			throw std::runtime_error("unable to read private key");
		else if (!EC_KEY_check_key(key))
			throw std::runtime_error("bogus key");
		else if ((decode_len =
		    crypt_get_base64_decode_length(challenge)) <= 0)
			throw std::runtime_error("decode length error");

		decoded_chl = static_cast<uint8_t *>(xmalloc(decode_len));
		decoded_chl[decode_len - 1] = '\0';

		if ((decode_ret = b64_decode(challenge, decoded_chl, static_cast
		    <size_t>(decode_len))) == -1)
			throw std::runtime_error("decoding error");
		else if (!sign_decoded_data(key, decoded_chl, decode_ret, &sig,
		    &siglen))
			throw std::runtime_error("signing error");
		else if ((encode_len = crypt_get_base64_encode_length
		    (static_cast<int>(siglen))) <= 0)
			throw std::runtime_error("encode length error");

		out = static_cast<char *>(xmalloc(encode_len));
		out[encode_len - 1] = '\0';

		if (b64_encode(sig, siglen, out, static_cast<size_t>
		    (encode_len)) == -1)
			throw std::runtime_error("encoding error");
	} catch (const std::runtime_error &e) {
		clean_up(key, fp, out, decoded_chl, sig);
		*err_reason = sw_strdup(e.what());
		return NULL;
	} catch (...) {
		clean_up(key, fp, out, decoded_chl, sig);
		*err_reason = sw_strdup("unknown exception was thrown!");
		return NULL;
	}

	clean_up(key, fp, NULL, decoded_chl, sig);
	*err_reason = NULL;
	return out;
}
