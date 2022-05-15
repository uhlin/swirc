#ifndef COMMAND_HELP_H
#define COMMAND_HELP_H
/* commandhelp.h
   Copyright (C) 2018-2022 Markus Uhlin. All rights reserved.

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

#include "i18n.h"

typedef const char *usage_t[];

//lint -e786

/*
 * set-fill-column: 70
 * Run esc+q on the text from the outside of the array
 */

static usage_t away_usage = {
  N_("usage: /away [reason]"),
  "",
  N_("Marks yourself as being away (with a reason). If the reason is omitted"),
  N_("you will be marked as no longer being away."),
  "",
};

static usage_t ban_usage = {
  N_("usage: /ban <mask>"),
  "",
  N_("Sets a channel ban which rejects all users whose nick!user@host"),
  N_("matches the provided mask from joining the channel. Wildcards are okay"),
  N_("and the active window must be an IRC channel."),
  "",
};

static usage_t banlist_usage = {
  N_("usage: /banlist [channel]"),
  "",
  N_("Outputs a channel's banlist. If the channel argument is left empty and"),
  N_("the active window is an IRC channel, Swirc will output the banlist for"),
  N_("that channel."),
  "",
};

static usage_t beep_usage = {
  N_("usage: /beep <nickname>"),
  "",
  N_("Send beeps. (ICB only)"),
  "",
};

static usage_t boot_usage = {
  N_("usage: /boot <victim>"),
  "",
  N_("Kick a user out of your group. (ICB only)"),
  "",
};

static usage_t chanserv_usage = {
  N_("usage: /chanserv <[service hostname | --]> <command> [...]"),
  "",
  N_("Communicate with your IRC network's channel service. If the initial"),
  N_("argument equals to '--', then the value of setting 'chanserv_host' is"),
  N_("used as a service hostname. See also '/nickserv'."),
  "",
};

static usage_t cleartoasts_usage = {
  N_("usage: /cleartoasts"),
  "",
  N_("On Windows Swirc sends toast notifications. By issuing this command"),
  N_("all notifications associated with Swirc will be cleared."),
  "",
};

static usage_t close_usage = {
  N_("usage: /close"),
  "",
  N_("Closes the active window. It's not possible to close the status"),
  N_("window. And while connected it's not possible to close a channel, in"),
  N_("that case instead use '/part'."),
  "",
};

static usage_t colormap_usage = {
  N_("usage: /colormap"),
  "",
  N_("Outputs information about colors."),
  "",
};

static usage_t connect_usage = {
  N_("usage: /connect [-tls] <server[:port]>"),
  "",
  N_("Connect to given server."),
  "",
  N_("If the port is omitted port 6667 will be chosen. And if the port is"),
  N_("7326 ICB mode is turned on automatically. Further, if the port is 6697"),
  N_("Swirc attempts to initiate a TLS/SSL connection, as well as if '-tls'"),
  N_("is present."),
  "",
  N_("It is possible to connect to a certain IRC network by only entering"),
  N_("the network name. For example: '/connect -tls libera', will connect to"),
  N_("Libera Chat using an encrypted connection. Preprogrammed network names"),
  N_("are:"),
  "",
  "    - afternet",
  "    - alphachat",
  "    - anonops",
  "    - blitzed",
  "    - efnet",
  "    - freenode",
  "    - ircnet",
  "    - libera",
  "    - quakenet",
  "    - undernet",
  "",
};

static usage_t cycle_usage = {
  N_("usage: /cycle [channel]"),
  "",
  N_("Cycle a channel, i.e. '/part' plus '/join'. If the channel argument is"),
  N_("omitted and the active window is an IRC channel, the client will cycle"),
  N_("that channel."),
  "",
};

static usage_t deop_usage = {
  N_("usage: /deop <nick>"),
  "",
  N_("Remove the channel operator privilege from another user."),
  "",
};

static usage_t disconnect_usage = {
  N_("usage: /disconnect [message]"),
  "",
  N_("Disconnect from IRC, but don't quit the program. A disconnect message"),
  N_("is optional."),
  "",
};

static usage_t echo_usage = {
  N_("usage: /echo <text>"),
  "",
  N_("Writes text to the current window without sending anything."),
  "",
  (TXT_BOLD "EXAMPLE" TXT_BOLD),
  "",
  N_("Echo 'Hello World':"),
  N_("    /echo Hello World"),
  "",
};

static usage_t exlist_usage = {
  N_("usage: /exlist [channel]"),
  "",
  N_("Outputs a channel's exception list. An exception mask (+e) overrides a"),
  N_("ban mask. If the channel argument is omitted and the active window is"),
  N_("an IRC channel, the client will output the exception list for that"),
  N_("channel."),
  "",
};

static usage_t group_usage = {
  N_("usage: /group <name>"),
  "",
  N_("Changes ICB group."),
  "",
};

static usage_t help_usage = {
  N_("usage: /help [command]"),
  "",
  N_("Outputs help. If a command is given the program will output help for"),
  N_("that particular command."),
  "",
};

static usage_t ignore_usage = {
  N_("usage: /ignore [regex]"),
  "",
  N_("Ignores all 'nick!user@host' that matches given regular expression,"),
  N_("this by using the basic POSIX regular expression grammar. This command"),
  N_("isn't to be used by beginners and I advice you to be careful when"),
  N_("using it. I highly recommend the use of:"),
  "",
  N_("    1. ^  Matches the starting position within the string, if it is"),
  N_("          the first character of the regular expression."),
  N_("    2. $  Matches the ending position of the string, if it is the last"),
  N_("          character of the regular expression."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("Ignore nickname 'troll':"),
  "    /ignore ^troll!.*@.*$",
  "",
  N_("Ignore all users with username 'troll':"),
  "    /ignore ^.*!troll@.*$",
  "",
  N_("Ignore all users with hostname 'insecure.org':"),
  "    /ignore ^.*!.*@insecure\\.org$",
  "",
  N_("Ignore all users with a Chinese domain (.cn):"),
  "    /ignore ^.*!.*@.*\\.cn$",
  "",
};

static usage_t ilist_usage = {
    "usage: /ilist [channel]",
    "",
    "Outputs channel invitation list. An invitation mask (+I)",
    "overrides the invite-only flag (+i). If channel is omitted and",
    "the active window is an IRC channel, it'll output the invitation",
    "list for that channel.",
    "",
};

static usage_t invite_usage = {
    "usage: /invite <targ_nick> <channel>",
    "",
    "Invites targ_nick to channel.",
    "",
};

static usage_t join_usage = {
    "usage: /join <channel> [key]",
    "",
    "Joins a channel (optionally by using a key).",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "Join a channel with name freenode:",
    "    /join #freenode",
    "",
    "Join a key protected channel:",
    "    /join #secret KeyToJoin",
    "",
};

static usage_t kick_usage = {
    "usage: /kick <nick1[,nick2][,nick3][...]> [reason]",
    "",
    "Kicks one or more users out of a channel as specified by a",
    "comma-separated list, and optionally with a reason. The active",
    "window must be an IRC channel.",
    "",
};

static usage_t kickban_usage = {
    "usage: /kickban <nick> <mask> [reason]",
    "",
    "Initially designate a channel ban specified by <mask> and kick",
    "user <nick> out of a channel. Optionally with a reason.",
    "(The active window must be an IRC channel.)",
    "",
};

static usage_t kill_usage = {
    "usage: /kill <nickname> <comment>",
    "",
    "Disconnect a user from the connected network.",
    "",
};

static usage_t list_usage = {
    "usage: /list [<max_users[,>min_users][,pattern][...]]",
    "",
    "List channels and their topics. Without any arguments the output",
    "is HUGE. For example, /list >1500 will only list channels that",
    "have more than 1500 users.",
    "",
};

static usage_t me_usage = {
    "usage: /me <message>",
    "",
    "Action message. Used to simulate role playing on IRC. The active",
    "window must be an IRC channel.",
    "",
};

static usage_t mode_usage = {
    "usage: /mode <modes> [...]",
    "",
    "Alter modes.",
    "",
    TXT_BOLD "CHANNEL MODES" TXT_BOLD,
    "",
    "    o - give/take channel operator privilege",
    "    v - give/take the voice privilege",
    "",
    "    i - invite-only channel",
    "    m - moderated channel",
    "    n - no messages to channel from clients on the outside",
    "    p - private channel",
    "    s - secret channel",
    "    t - topic settable by channel operators only",
    "",
    "    k - set/remove the channel key (password)",
    "    l - set/remove the user limit to channel",
    "",
    "    b - set/remove ban mask to keep users out",
    "    e - set/remove an exception mask to override a ban mask",
    "    I - set/remove an invitation mask to automatically override",
    "        the invite-only flag",
    "",
    TXT_BOLD "USER MODES" TXT_BOLD,
    "",
    "    i - marks a user as invisible",
    "    w - user receives wallops",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "    Give channel operator privilege to Companion on #foo:",
    "      /mode #foo +o Companion",
    "",
    "    Restrict messaging to channel #linux:",
    "      /mode #linux +n",
    "",
    "    Limit user count for #freenode to 10:",
    "      /mode #freenode +l 10",
    "",
    "    Deny all users with hostname spammers.net from joining",
    "    #chatzone:",
    "      /mode #chatzone +b *!*@spammers.net",
    "",
    "    Turn on reception of WALLOPS messages:",
    "      /mode MyNickname +w",
    "",
    /*
     * TODO: Sync with cmds.html
     */
};

static usage_t msg_usage = {
    "usage: /msg <recipient> <message>",
    "",
    "Used to send private messages between users, as well as to send",
    "messages to channels.",
    "",
};

static usage_t nick_usage = {
    "usage: /nick <new nickname>",
    "",
    "Set nickname.",
    "",
};

static usage_t nickserv_usage = {
    "usage: /nickserv <[service hostname | --]> <command> [...]",
    "",
    "Communicate with nickname services.",
    "",
    "If the first argument is '--' then the:",
    "    - Value of configuration option nickserv_host is used as",
    "      service hostname",
    "    - Command call won't be added to the command history provided",
    "      that the second argument is identify",
    "",
    "The correct service hostname is not always the same as the",
    "visible hostname of NickServ. FYI at the AnonOps IRC network the",
    "visible hostname of NickServ is anonops.in (at the time of",
    "writing this) but services.anonops.com is urged to be used. As a",
    "fallback: use /query NickServ in order to communicate with the",
    "service.",
    "",
};

static usage_t notice_usage = {
    "usage: /notice <recipient> <message>",
    "",
    "Used to send private messages between users, as well as to send",
    "messages to channels. (In notice form).",
    "",
};

static usage_t op_usage = {
    "usage: /op <nick>",
    "",
    "Give channel operator privilege.",
    "",
};

static usage_t oper_usage = {
    "usage: /oper <name> <password>",
    "",
    "Identify as an IRC op.",
    "",
};

static usage_t part_usage = {
    "usage: /part [channel] [message]",
    "",
    "Parts a channel with an optional message. For a standard channel a",
    "leading hash (#) must be present. If the command is called without",
    "any arguments Swirc attempts to part the current window which must",
    "be an IRC channel.",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "  Part channel #chatzone with message "
    TXT_UNDERLINE "bye" TXT_UNDERLINE ":",
    "    /part #chatzone bye",
    "",
};

static usage_t passmod_usage = {
    "usage: /passmod <nickname>",
    "",
    "Pass moderation privilege.",
    "In IRC mode this is a no operation.",
    "",
};

static usage_t query_usage = {
    "usage: /query [nick]",
    "",
    "Start a query with nick. If nick is omitted, and provided that",
    "the active window is a query, close the query.",
    "",
};

static usage_t quit_usage = {
    "usage: /quit [message]",
    "",
    "Disconnect from IRC and quit the program. A disconnect message is",
    "optional.",
    "",
};

static usage_t resize_usage = {
    "usage: /resize",
    "",
    "Resize the terminal. For example, Windows doesn't send SIGWINCH,",
    "instead this command can be used. First resize the window then",
    "issue this command.",
    "",
};

#if NOT_YET
static usage_t rgui_usage = {
    "usage: /rgui ...",
    "",
    "No help yet.",
    "",
};
#endif

static usage_t rules_usage = {
    "usage: /rules",
    "",
    "Outputs network/server rules. Not all IRCd:s supports this",
    "command.",
    "",
};

static usage_t sasl_usage = {
    "usage: /sasl <operation> [...]",
    "",
    "Simple Authentication and Security Layer.",
    "Operation can be either:",
    "",
    "    keygen [--force]",
    "    pubkey",
    "    mechanism [ecdsa-nist256p-challenge | plain | scram-sha-256]",
    "    username <name>",
    "    password <pass>",
    "    set [on | off]",
    "",
    "SASL is a method that allows identification to NickServ during",
    "the connection process eliminating the need to do it manually.",
    "(To use SASL, you must register your nickname.)",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "  Identification using mechanism ecdsa-nist256p-challenge:",
    "    1) /sasl keygen",
    "    2) /sasl pubkey (print it)",
    "    3) /ns <services hostname> set pubkey <public key>",
    "    4) /sasl mechanism ecdsa-nist256p-challenge",
    "    5) /sasl username <your nickserv username>",
    "    6) /sasl password dummy",
    "    7) /sasl set on",
    "  (The only IRC network that I know of that is supporting this",
    "  mechanism is freenode.)",
    "",
    "  Identification using mechanism plain:",
    "    1) /sasl mechanism plain",
    "    2) /sasl username <your nickserv username>",
    "    3) /sasl password <your nickserv password>",
    "    4) /sasl set on",
    "",
};

static usage_t say_usage = {
    "usage: /say <message>",
    "",
    "Say a message. This command can be used if you want say something",
    "with a leading command-character, i.e. a slash.",
    "",
};

static usage_t servlist_usage = {
    "usage: /servlist [<mask> [<type>]]",
    "",
    "This command is used to list services currently connected to the network",
    "and visible to the user issuing the command. The optional parameters may",
    "be used to restrict the result of the query (to matching services names,",
    "and services type).",
    "",
};

static usage_t set_usage = {
    "usage: /set [[setting] [value]]",
    "",
    "Alter Swirc settings.",
    "",
    TXT_BOLD "SETTING TYPES" TXT_BOLD,
    "",
    TXT_BOLD "bool" TXT_BOLD "      "
    "Bools are case insensitive and can have one of the",
    "          "
    "following values:",
    "          "
    "- on, true or yes",
    "          "
    "- off, false or no",
    "",
    TXT_BOLD "int" TXT_BOLD "       "
    "Integers. Swirc implements a min/max value for each",
    "          "
    "integer in order to keep its value safe. The error log",
    "          "
    "will tell if the restrictions for an integer aren't",
    "          "
    "within limits and, if so, that a preprogrammed fallback",
    "          "
    "value is being used instead.",
    "",
    TXT_BOLD "string" TXT_BOLD "    "
    "A sequence of characters",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "  Output current values of all settings:",
    "    /set (without any arguments)",
    "",
    "  Turn beeps on/off:",
    "    /set beeps on",
    "    /set beeps off",
    "",
    /*
     * TODO: Sync with cmds.html
     */
};

static usage_t squery_usage = {
    "usage: /squery <servicename> <text>",
    "",
    "This command is used similarly to '/msg'. The only difference is that the",
    "recipient MUST be a service.",
    "",
};

static usage_t theme_usage = {
    "usage: /theme [install <name> | list-remote | set <name>]",
    "",
    "Management of themes on-the-fly.",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "    Install theme named 'bx':",
    "        /theme install bx",
    "",
    "    List available themes:",
    "        /theme list-remote",
    "",
    "    Activate installed theme:",
    "        /theme set bx",
    "",
};

static usage_t time_usage = {
    "usage: /time <target>",
    "",
    "Send a CTCP TIME request to target, which is either a nickname or",
    "an IRC channel.",
    "",
};

static usage_t topic_usage = {
    "usage: /topic [new topic]",
    "",
    "Set a new topic for a channel. If new topic is omitted, display",
    "the current topic. Active window must be an IRC channel.",
    "",
};

static usage_t unban_usage = {
    "usage: /unban <mask>",
    "",
    "Unset a ban. (The active window must be an IRC channel.)",
    "",
};

static usage_t unignore_usage = {
    "usage: /unignore [#]",
    "",
    "Delete a regular expression from the ignore list.",
    "",
};

static usage_t version_usage = {
    "usage: /version <target>",
    "",
    "Send a CTCP VERSION request to target, which is either a nickname",
    "or an IRC channel.",
    "",
};

static usage_t who_usage = {
    "usage: /who <mask>",
    "",
    "Used by a client to generate a query which returns a list of",
    "information which 'matches' the mask parameter given by the",
    "client.",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "  Show all staff:",
    "    /who freenode/staff/*",
    "",
    "  Show ppl with a german domain:",
    "    /who *.de",
    "",
    /*
     * TODO: Sync with cmds.html
     */
};

static usage_t whois_usage = {
    "usage: /whois <nick>",
    "",
    "Used to query information about particular user.",
    "(Specified by nick)",
    "",
};

static usage_t znc_usage = {
    "usage: /znc [*module] <command>",
    "",
    "This command simplifies communication with ZNC (an IRC bouncer).",
    "",
    TXT_BOLD "EXAMPLES" TXT_BOLD,
    "",
    "  Output ZNC version:",
    "    /znc version",
    "",
    "  Same as above:",
    "    /znc *status version",
    "",
};

//lint +e786

#endif
