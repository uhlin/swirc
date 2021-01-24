/* commands/sasl.c
   Copyright (C) 2017-2021 Markus Uhlin. All rights reserved.

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

#include "../irc.h"
#include "../events/cap.h"

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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window,
	is_error ? TYPE_SPEC1_FAILURE : TYPE_SPEC1_SUCCESS, true);
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
    } else if ((fp = xfopen(path, "w")) == NULL) {
	output_message(true, "xfopen failed");
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
sasl_pubkey(void)
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
    } else if ((fp = xfopen(path, "r")) == NULL) {
	output_message(true, "pubkey: xfopen failed");
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

    char *msg = strdup_printf("public key is: %s", buf);
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
save_to_config(void)
{
    char path[2300] = { '\0' };
    size_t len = 0;

    if (g_home_dir == NULL)
	return;

    len += strlen(g_home_dir);
    len += strlen(SLASH);
    len += strlen("swirc");
    len += strlen(g_config_filesuffix);

    if (len >= sizeof path)
	return;

    (void) sw_strcpy(path, g_home_dir, sizeof path);
    (void) sw_strcat(path, SLASH,      sizeof path);
    (void) sw_strcat(path, "swirc",    sizeof path);
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
    (void) strToLower(state);

    if (!strings_match(state, "on") && !strings_match(state, "off")) {
	output_message(true, "what? on or off?");
	return;
    } else if (!modify_setting("sasl", state)) {
	output_message(true, "set sasl on/off failed");
	return;
    } else {
	char *msg = strdup_printf("SASL authentication is now %s",
	    strings_match(state, "on") ? "ON" : "OFF");
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
   set [on | off] */
void
cmd_sasl(const char *data)
{
    char mechanism[31] = { '\0' };
    char username[101] = { '\0' };
    char password[301] = { '\0' };
    char state[11]     = { '\0' };

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
    unsigned int len = 0;

    if (!key || (len = (unsigned int) ECDSA_size(key)) == 0) {
	*sig = NULL;
	*siglen = 0;
	return false;
    }

    *sig = xmalloc(len);

    if (!ECDSA_sign(0, data, datalen, *sig, &len, key)) {
	free(*sig);
	*sig = NULL;
	*siglen = 0;
	return false;
    }

    *siglen = len;
    return true;
}

char *
solve_ecdsa_nist256p_challenge(const char *challenge, char **err_reason)
{
    EC_KEY       *key       = NULL;
    FILE         *fp        = NULL;
    char         *path      = get_filepath(false);
    int           datalen   = -1;
    uint8_t      *sig       = NULL;
    uint8_t       buf[2000] = { '\0' };
    unsigned int  siglen    = 0;

    if ((key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL) {
	*err_reason = "EC_KEY_new_by_curve_name failed";
	return NULL;
    } else {
	EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    }

    if (!path) {
	*err_reason = "unable to get file path";
	goto err;
    } else if (!file_exists(path)) {
	*err_reason = "unable to locate private key";
	goto err;
    } else if ((fp = xfopen(path, "r")) == NULL) {
	*err_reason = "xfopen failed";
	goto err;
    } else if (PEM_read_ECPrivateKey(fp, &key, NULL, NULL) == NULL) {
	*err_reason = "PEM_read_ECPrivateKey failed";
	goto err;
    } else if (!EC_KEY_check_key(key)) {
	*err_reason = "EC_KEY_check_key failed";
	goto err;
    } else if ((datalen = b64_decode(challenge, buf, sizeof buf)) == -1) {
	*err_reason = "b64_decode failed";
	goto err;
    } else if (!sign_decoded_data(key, buf, datalen, &sig, &siglen)) {
	*err_reason = "sign_decoded_data failed";
	goto err;
    } else if (BZERO(buf, sizeof buf),
	       b64_encode(sig, siglen, (char *) buf, sizeof buf) == -1) {
	*err_reason = "b64_encode failed";
	goto err;
    }

    *err_reason = "";
    EC_KEY_free(key);
    fclose(fp);
    free(sig);
    return (sw_strdup((char *) buf));

  err:
    if (key)
	EC_KEY_free(key);
    if (fp)
	fclose(fp);
    if (sig)
	free(sig);
    return NULL;
}
