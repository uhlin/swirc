/* commands/sasl.c
   Copyright (C) 2017 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

#include "../assertAPI.h"
#include "../config.h"
#include "../filePred.h"
#include "../libUtils.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"

#include "sasl.h"

#if defined(UNIX)
#define SLASH "/"
#elif defined(WIN32)
#define SLASH "\\"
#endif

/*lint -sem(get_filepath, r_null) */
static char *
get_filepath(const bool is_public)
{
    static char path[2300] = { '\0' };

    if (sw_strcpy(path, g_home_dir, sizeof path) != 0)
	return NULL;
    else if (sw_strcat(path, SLASH, sizeof path) != 0)
	return NULL;
    else if (sw_strcat(path, (is_public ? "ec_key.pub" : "ec_key"),
		       sizeof path) != 0)
	return NULL;
    else
	return (&path[0]);

    /*NOTREACHED*/ sw_assert_not_reached();
    /*NOTREACHED*/ return NULL;
}

static void
output_message(const bool is_error, const char *message)
{
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = is_error ? TYPE_SPEC1_FAILURE : TYPE_SPEC1_SUCCESS,
	.include_ts = true,
    };

    printtext(&ctx, "%s", message);
}

static void
sasl_keygen(const bool force)
{
    EC_KEY *key  = NULL;
    FILE   *fp   = NULL;
    char   *path = get_filepath(false);

    if (!path) {
	output_message(true, "unable to get file path");
	return;
    } else if (file_exists(path) && !force) {
	output_message(true, "file exists! add --force to overwrite it");
	return;
    } else if ((key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL) {
	output_message(true, "EC_KEY_new_by_curve_name failed");
	return;
    } else {
	EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    }

    if (!EC_KEY_generate_key(key)) {
	output_message(true, "EC_KEY_generate_key failed");
	goto err;
    } else if ((fp = fopen(path, "w")) == NULL) {
	output_message(true, "fopen failed");
	goto err;
    } else if (!PEM_write_ECPrivateKey(fp, key, NULL, NULL, 0, NULL, NULL)) {
	output_message(true, "PEM_write_ECPrivateKey failed");
	goto err;
    }

    output_message(false, "key generation successful");

  err:
    if (key)
	EC_KEY_free(key);
    if (fp)
	fclose(fp);
}

static int
public_key_length(EC_KEY *key)
{
    if (!key)
	return -1;
    return (i2o_ECPublicKey(key, NULL));
}

/*lint -sem(public_key_blob, r_null) */
static unsigned char *
public_key_blob(EC_KEY *key)
{
    int length = -1;
    unsigned char *out = NULL, *out_p = NULL;

    if (key == NULL || (length = public_key_length(key)) <= 0)
	return NULL;
    out = xmalloc(length);
    out_p = &out[0];
    if (i2o_ECPublicKey(key, &out_p) < 0) {
	free(out);
	return NULL;
    }
    /*out_p = &out[0]*/
    return (out);
}

static void
sasl_pubkey()
{
    EC_KEY        *key       = NULL;
    FILE          *fp        = NULL;
    char          *path      = get_filepath(false);
    char           buf[2000] = { '\0' };
    int            length    = -1;
    unsigned char *blob      = NULL;

    if ((key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL) {
	output_message(true, "pubkey: EC_KEY_new_by_curve_name failed");
	return;
    } else {
	EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    }

    if (!path) {
	output_message(true, "pubkey: unable to get file path");
	goto err;
    } else if (!file_exists(path)) {
	output_message(true, "pubkey: unable to locate private key "
	    "(doesn't exist  --  use keygen)");
	goto err;
    } else if ((fp = fopen(path, "r")) == NULL) {
	output_message(true, "pubkey: fopen failed");
	goto err;
    } else if (PEM_read_ECPrivateKey(fp, &key, NULL, NULL) == NULL) {
	output_message(true, "pubkey: PEM_read_ECPrivateKey failed");
	goto err;
    } else if (/*EC_KEY_set_conv_form()*/ !EC_KEY_check_key(key)) {
	output_message(true, "pubkey: EC_KEY_check_key failed");
	goto err;
    } else if ((length = public_key_length(key)) <= 0) {
	output_message(true, "pubkey: public_key_length failed");
	goto err;
    } else if ((blob = public_key_blob(key)) == NULL) {
	output_message(true, "pubkey: public_key_blob failed");
	goto err;
    } else if (b64_encode(blob, length, buf, sizeof buf) == -1) {
	output_message(true, "pubkey: b64_encode failed");
	goto err;
    }

    char *msg = Strdup_printf("public key is: %s", buf);
    output_message(false, msg);
    free(msg);

  err:
    if (key)
	EC_KEY_free(key);
    if (fp)
	fclose(fp);
    if (blob)
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
save_to_config()
{
    char path[2300] = { '\0' };

    (void) sw_strcpy(path, g_home_dir, sizeof path);
    (void) sw_strcat(path, SLASH,      sizeof path);
    (void) sw_strcat(path, "swirc",    sizeof path);
    (void) sw_strcat(path, g_config_filesuffix, sizeof path);

    config_do_save(path, "w");
}

static void
set_mechanism(char *mechanism)
{
    extern bool is_sasl_mechanism_supported(const char *); /*from events/cap.c*/

    (void) str_toupper(mechanism);

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
set_username(char *username)
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
set_password(char *password)
{
    if (!modify_setting("sasl_password", password)) {
	output_message(true, "set password failed");
	return;
    } else {
	output_message(false, "set password ok");
	save_to_config();
    }
}

static void
set_state(char *state)
{
    (void) str_tolower(state);

    if (!Strings_match(state, "on") && !Strings_match(state, "off")) {
	output_message(true, "what? on or off?");
	return;
    } else if (!modify_setting("sasl", state)) {
	output_message(true, "set sasl on/off failed");
	return;
    } else {
	char *msg = Strdup_printf("SASL authentication is now %s",
	    Strings_match(state, "on") ? "ON" : "OFF");
	output_message(false, msg);
	free(msg);
	save_to_config();
    }
}

/* usage: /sasl <operation> [...]

   keygen [--force]
   pubkey
   mechanism [ecdsa-nist256p-challenge | plain]
   username <name>
   password <pass>
   set [on | off] */
void
cmd_sasl(const char *data)
{
    char mechanism[31] = { '\0' };
    char username[101] = { '\0' };
    char password[301] = { '\0' };
    char state[11]     = { '\0' };

    if (Strings_match(data, "keygen") || Strings_match(data, "KEYGEN"))
	sasl_keygen(false);
    else if (Strings_match(data, "keygen --force") ||
	     Strings_match(data, "KEYGEN --force"))
	sasl_keygen(true);
    else if (Strings_match(data, "pubkey") || Strings_match(data, "PUBKEY"))
	sasl_pubkey();
    else if (sscanf(data, "mechanism %30s", mechanism) == 1)
	set_mechanism(mechanism);
    else if (sscanf(data, "username %100s", username) == 1)
	set_username(username);
    else if (sscanf(data, "password %300s", password) == 1)
	set_password(password);
    else if (sscanf(data, "set %10s", state) == 1)
	set_state(state);
    else
	output_message(true, "bogus operation");
}
