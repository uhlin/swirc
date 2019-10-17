#include "common.h"

#include "icb.h"
#include "irc.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

static char	 icb_protolevel[256] = { '\0' };
static char	 icb_hostid[256]     = { '\0' };
static char	 icb_serverid[256]   = { '\0' };
static char	*icb_group           = NULL;

/*
 * "It's perfectly appropriate to parse strings with sscanf (as long
 * as the return value is checked), because it's so easy to regain
 * control, restart the scan, discard the input if it didn't match,
 * etc."  --  c-faq.com/stdio/scanfprobs.html
 *
 * NOTE: sscanf() not scanf()
 */

/*lint -printf(1, process_event) */

static void
process_event(const char *format, ...)
{
    char *event = NULL;
    char *message_concat = NULL;
    enum message_concat_state state = CONCAT_BUFFER_IS_EMPTY;
    va_list ap;

    va_start(ap, format);
    event = strdup_vprintf(format, ap);
    va_end(ap);

    irc_handle_interpret_events(event, &message_concat, &state);
    free(event);
}

static void
login_ok()
{
    if (strings_match(icb_protolevel, "") || strings_match(icb_hostid, "") ||
	strings_match(icb_serverid, "")) {
	print_and_free("login_ok: empty protocol level, host id or server id",
	    NULL);
	g_on_air = false;
	return;
    }

    /*
     * 001: RPL_WELCOME
     */
    process_event(":%s 001 %s :Welcome to ICB, %s!\r\n", icb_hostid,
	g_my_nickname, g_my_nickname);

    /*
     * 002: RPL_YOURHOST
     */
    process_event(":%s 002 %s :Your host is %s, running version %s\r\n",
	icb_hostid, g_my_nickname, icb_serverid, icb_protolevel);

    /*
     * 375: RPL_MOTDSTART
     */
    process_event(":%s 375 %s :- %s Message Of The Day -\r\n", icb_hostid,
	g_my_nickname, icb_serverid);

    /*
     * 372: RPL_MOTD
     */
    process_event(
	":%s 372 %s :-----------------------------\r\n"
	":%s 372 %s :   Internet Citizen's Band   \r\n"
	":%s 372 %s :-----------------------------\r\n",
	icb_hostid, g_my_nickname,
	icb_hostid, g_my_nickname,
	icb_hostid, g_my_nickname);

    /*
     * 376: RPL_ENDOFMOTD
     */
    process_event(":%s 376 %s :End of MOTD\r\n", icb_hostid, g_my_nickname);
}

static void
handle_personal_msg_packet(const char *pktdata)
{
    char *last = "";
    char *message = NULL;
    char *nickname = NULL;
    char *pktdata_copy = sw_strdup(pktdata);

    nickname = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last);
    message = strtok_r(NULL, ICB_FIELD_SEP, &last);

    if (nickname == NULL || message == NULL) {
	print_and_free("handle_personal_msg_packet: too few tokens!",
	    pktdata_copy);
	return;
    }

    process_event(":%s PRIVMSG %s :%s\r\n", nickname, g_my_nickname, message);
    free_and_null(&pktdata_copy);
}

static void
handle_status_msg_packet(const char *pktdata)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = NULL;
    char *pktdata_copy = sw_strdup(pktdata);

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

    if (!strncmp(pktdata_copy, "No-Pass" ICB_FIELD_SEP, 8)) {
	printtext(&ctx, "%s", &pktdata_copy[8]);
    } else if (!strncmp(pktdata_copy, "Status" ICB_FIELD_SEP, 7)) {
	cp = &pktdata_copy[7];

	if (!strncmp(cp, "You are now in group ", 21)) {
	    if (icb_group)
		process_event(":%s PART #%s\r\n", g_my_nickname, icb_group);

	    cp += 21;
	    free_and_null(&icb_group);
	    icb_group = sw_strdup(cp);

	    process_event(":%s JOIN :#%s\r\n", g_my_nickname, cp);

	    icb_send_users(cp);
	}
    } else {
	while ((cp = strpbrk(pktdata_copy, ICB_FIELD_SEP)) != NULL)
	    *cp = 'X';
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "handle_status_msg_packet: "
	    "unknown status message category");
	printtext(&ctx, "packet data: %s", pktdata_copy);
    }

    free_and_null(&pktdata_copy);
}

static void
handle_cmd_output_packet(const char *pktdata)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = NULL;
    char *last = "";
    char *pktdata_copy = sw_strdup(pktdata);

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

    if (!strncmp(pktdata_copy, "co", 2)) {
	/*
	 * Generic command output
	 */

	squeeze(pktdata_copy, ICB_FIELD_SEP);
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s", &pktdata_copy[2]);
    } else if (!strncmp(pktdata_copy, "wl", 2)) {
	char *initial_token = strtok_r(&pktdata_copy[2], ICB_FIELD_SEP, &last);
	char *nickname      = strtok_r(NULL, ICB_FIELD_SEP, &last);
#if 0
	char *seconds_idle  = strtok_r(NULL, ICB_FIELD_SEP, &last);
	char *response_time = strtok_r(NULL, ICB_FIELD_SEP, &last);
	char *login_time    = strtok_r(NULL, ICB_FIELD_SEP, &last);
	char *username      = strtok_r(NULL, ICB_FIELD_SEP, &last);
	char *userhost      = strtok_r(NULL, ICB_FIELD_SEP, &last);
	char *reg_status    = strtok_r(NULL, ICB_FIELD_SEP, &last);
#endif

	if (initial_token == NULL || nickname == NULL) {
	    ctx.spec_type = TYPE_SPEC1_FAILURE;
	    printtext(&ctx, "handle_cmd_output_packet: missing essential "
		"initial token or nickname during who listing");
	} else {
	    const bool is_moderator = !strings_match(initial_token, " ");

	    process_event(":%s 353 %s = #%s :%s%s\r\n", icb_hostid,
		g_my_nickname, icb_group, is_moderator ? "@" : "", nickname);
	}
    } else {
	/*
	 * Unknown output type
	 */

	while ((cp = strpbrk(pktdata_copy, ICB_FIELD_SEP)) != NULL)
	    *cp = 'X';
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "handle_cmd_output_packet: unknown output type");
	printtext(&ctx, "data: %s", pktdata_copy);
    } /* if-then-else */

    free_and_null(&pktdata_copy);
}

static void
handle_proto_packet(const char *pktdata)
{
    char *cp = NULL;
    char *last = "";
    char *pktdata_copy = sw_strdup(pktdata);

    if ((cp = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last)) != NULL)
	(void) sw_strcpy(icb_protolevel, cp, ARRAY_SIZE(icb_protolevel));
    if ((cp = strtok_r(NULL, ICB_FIELD_SEP, &last)) != NULL)
	(void) sw_strcpy(icb_hostid, cp, ARRAY_SIZE(icb_hostid));
    if ((cp = strtok_r(NULL, ICB_FIELD_SEP, &last)) != NULL)
	(void) sw_strcpy(icb_serverid, cp, ARRAY_SIZE(icb_serverid));

    free_and_null(&pktdata_copy);

    if (strings_match(icb_protolevel, "") || strings_match(icb_hostid, "") ||
	strings_match(icb_serverid, "")) {
	memset(icb_protolevel, 0, ARRAY_SIZE(icb_protolevel));
	memset(icb_hostid, 0, ARRAY_SIZE(icb_hostid));
	memset(icb_serverid, 0, ARRAY_SIZE(icb_serverid));
    }
}

static void
unknown_packet_type(const char length, const char type, const char *pktdata)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "unknown_packet_type: '%c' (length: %d): %s", type, length,
	pktdata);
}

void
icb_irc_proxy(char length, char type, const char *pktdata)
{
    switch (type) {
    case 'a':
	login_ok();
	break;
    case 'c':
	handle_personal_msg_packet(pktdata);
	break;
    case 'd':
	handle_status_msg_packet(pktdata);
	break;
    case 'i':
	handle_cmd_output_packet(pktdata);
	break;
    case 'j':
	handle_proto_packet(pktdata);
	break;
    default:
	unknown_packet_type(length, type, pktdata);
	break;
    }
}

void
icb_send_group(const char *group)
{
    char packet[ICB_PACKET_MAX] = { '\0' };

    snprintf(packet, ARRAY_SIZE(packet), " hg%s%s", ICB_FIELD_SEP, group);
    packet[0] = (char) strlen(&packet[1]);
    net_send("%s", packet);
}

void
icb_send_users(const char *arg)
{
    char packet[ICB_PACKET_MAX] = { '\0' };

    snprintf(packet, ARRAY_SIZE(packet), " hw%s%s", ICB_FIELD_SEP, arg);
    packet[0] = (char) strlen(&packet[1]);
    net_send("%s", packet);
}
