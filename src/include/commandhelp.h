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
  N_("used as a service hostname."),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /nickserv",
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
  N_("usage: /ilist [channel]"),
  "",
  N_("Outputs a channel's invitation list. An invitation mask (+I) overrides"),
  N_("the invite-only flag (+i). If the channel argument is omitted and the"),
  N_("active window is an IRC channel, the client will output the invitation"),
  N_("list for that channel."),
  "",
};

static usage_t invite_usage = {
  N_("usage: /invite <targ_nick> <channel>"),
  "",
  N_("Invites 'targ_nick' to a channel."),
  "",
};

static usage_t join_usage = {
  N_("usage: /join <channel> [key]"),
  "",
  N_("Joins a channel (optionally by using a key)."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("Join a channel with name 'libera':"),
  "    /join #libera",
  "",
  N_("Join a key-protected channel:"),
  "    /join #secret KeyToJoin",
  "",
};

static usage_t kick_usage = {
  N_("usage: /kick <nick1[,nick2][,nick3][...]> [reason]"),
  "",
  N_("Kicks one or more users out of a channel (specified by a comma-"),
  N_("separated list) and optionally with a reason. The active window must"),
  N_("be an IRC channel."),
  "",
};

static usage_t kickban_usage = {
  N_("usage: /kickban <nick> <mask> [reason]"),
  "",
  N_("Initially set a channel ban by given 'mask' and kick user 'nick' out"),
  N_("of a channel, optionally with a reason."),
  N_("(The active window must be an IRC channel.)"),
  "",
};

static usage_t kill_usage = {
  N_("usage: /kill <nickname> <comment>"),
  "",
  N_("Disconnect a user from the connected network."),
  N_("(Requires IRC op privilege.)"),
  "",
};

static usage_t list_usage = {
  N_("usage: /list [<max_users[,>min_users][,pattern][...]]"),
  "",
  N_("Lists channels and their topics. Without any arguments the output is"),
  N_("HUGE. For example, '/list >1500' will only list channels that have"),
  N_("more than 1500 users."),
  "",
  N_("Depending on the IRC server software used by your network the usage"),
  N_("may differ."),
  "",
};

static usage_t me_usage = {
  N_("usage: /me <message>"),
  "",
  N_("Send an 'action' message. (Used to simulate role playing on IRC.)"),
  "",
};

static usage_t mode_usage = {
  N_("usage: /mode <modes> [...]"),
  "",
  N_("Alter modes."),
  "",
  (TXT_BOLD "CHANNEL MODES" TXT_BOLD),
  "",
  N_("    o - give/take the channel operator privilege"),
  N_("    v - give/take the channel voice privilege"),
  "",
  N_("    i - invite-only channel"),
  N_("    m - moderated channel"),
  N_("    n - no messages to a channel from clients on the outside"),
  N_("    p - private channel"),
  N_("    s - secret channel"),
  N_("    t - topic settable by channel operators only"),
  "",
  N_("    k - set/remove the channel key (password)"),
  N_("    l - set/remove the channel user limit"),
  "",
  N_("    b - set/remove a ban mask"),
  N_("    e - set/remove an exception mask to override a ban mask"),
  N_("    I - set/remove an invitation mask to override an invite-only flag"),
  "",
  (TXT_BOLD "USER MODES" TXT_BOLD),
  "",
  N_("    i - marks a user as invisible"),
  N_("    w - user receives wallops"),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("    Give channel operator privilege to 'Companion' on #foo:"),
  "      /mode #foo +o Companion",
  "",
  N_("    Restrict messaging to channel #linux:"),
  "      /mode #linux +n",
  "",
  N_("    Limit user count for #freenode to 10:"),
  "      /mode #freenode +l 10",
  "",
  N_("    Deny all users with hostname spammers.net from joining #chatzone:"),
  "      /mode #chatzone +b *!*@spammers.net",
  "",
  N_("    Turn on reception of WALLOPS messages:"),
  "      /mode MyNickname +w",
  "",
};

static usage_t msg_usage = {
  N_("usage: /msg <recipient> <message>"),
  "",
  N_("Used to send private messages between users, as well as to send"),
  N_("messages to channels."),
  "",
};

static usage_t nick_usage = {
  N_("usage: /nick <new nickname>"),
  "",
  N_("Sets your nickname."),
  "",
};

static usage_t nickserv_usage = {
  N_("usage: /nickserv <[service hostname | --]> <command> [...]"),
  "",
  N_("Communicate with your IRC network's nickname service."),
  "",
  N_("If the initial argument equals to '--', then the:"),
  "",
  N_("    1) Value of option 'nickserv_host' is used as service hostname."),
  N_("    2) Command call won't be added to the command history provided"),
  N_("       that the second argument is 'identify'."),
  "",
  N_("The correct service hostname is not always the same as the visible"),
  N_("hostname of NickServ. FYI at the AnonOps IRC network the visible"),
  N_("hostname of NickServ is anonops.in (at the time of writing this)"),
  N_("but you should use 'services.anonops.com'. As a fallback:"),
  N_("'/query NickServ' can be used in order to communicate with the"),
  N_("service."),
  "",
};

static usage_t notice_usage = {
  N_("usage: /notice <recipient> <message>"),
  "",
  N_("Used to send private messages between users, as well as to send"),
  N_("messages to channels. (In notice form.)"),
  "",
};

static usage_t op_usage = {
  N_("usage: /op <nick>"),
  "",
  N_("Gives the channel operator privilege to another user."),
  "",
};

static usage_t oper_usage = {
  N_("usage: /oper <name> <password>"),
  "",
  N_("Identifies yourself as an IRC operator."),
  "",
};

static usage_t part_usage = {
  N_("usage: /part [channel] [message]"),
  "",
  N_("Leaves a channel (optionally with a message). If the command is called"),
  N_("without any arguments and the current window is an IRC channel, that"),
  N_("channel will be the target."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Part channel #chatzone with message 'bye':"),
  N_("    /part #chatzone bye"),
  "",
};

static usage_t passmod_usage = {
  N_("usage: /passmod <nickname>"),
  "",
  N_("Pass the ICB moderation privilege to another group member."),
  "",
};

static usage_t query_usage = {
  N_("usage: /query [nick]"),
  "",
  N_("Starts a private conversation with 'nick'. If 'nick' is omitted and"),
  N_("the active window is a private conversation, the action will be to"),
  N_("close it."),
  "",
};

static usage_t quit_usage = {
  N_("usage: /quit [message]"),
  "",
  N_("Disconnect from IRC and quit the program. A disconnect message is"),
  N_("optional."),
  "",
};

static usage_t resize_usage = {
  N_("usage: /resize"),
  "",
  N_("Resize the terminal. For example, Windows doesn't send 'SIGWINCH',"),
  N_("instead this command can be used. First resize the window then run"),
  N_("this command."),
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
  N_("usage: /rules"),
  "",
  N_("Outputs network/server rules. Not all IRC server software supports"),
  N_("this command. (It's actually quite rare.)"),
  "",
};

static usage_t sasl_usage = {
  N_("usage: /sasl <operation> [...]"),
  "",
  N_("Simple Authentication and Security Layer."),
  N_("Operation can be either:"),
  "",
  "    keygen [--force]",
  "    pubkey",
  "    mechanism [ecdsa-nist256p-challenge | plain | scram-sha-256]",
  "    username <name>",
  "    password <pass>",
  "    set [on | off]",
  "",
  N_("SASL is a method that allows identification to NickServ during the"),
  N_("connection process eliminating the need to do it manually."),
  N_("(To use SASL you must register your nickname.)"),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Identification using mechanism ecdsa-nist256p-challenge:"),
  "    1) /sasl keygen",
  "    2) /sasl pubkey (print it)",
  "    3) /ns <services hostname> set pubkey <public key>",
  "    4) /sasl mechanism ecdsa-nist256p-challenge",
  "    5) /sasl username <your nickserv username>",
  "    6) /sasl password dummy",
  "    7) /sasl set on",
  N_("  (The only IRC network that I know of that is supporting this"),
  N_("  mechanism is Libera Chat.)"),
  "",
  N_("  Identification using mechanism plain:"),
  "    1) /sasl mechanism plain",
  "    2) /sasl username <your nickserv username>",
  "    3) /sasl password <your nickserv password>",
  "    4) /sasl set on",
  "",
};

static usage_t say_usage = {
  N_("usage: /say <message>"),
  "",
  N_("Say a message. This command can be used if you want say something with"),
  N_("a leading command-character, i.e. a slash."),
  "",
  (TXT_BOLD "EXAMPLE" TXT_BOLD),
  "",
  N_("    /say // A single line comment in C++"),
  "",
};

static usage_t servlist_usage = {
  N_("usage: /servlist [<mask> [<type>]]"),
  "",
  N_("Lists services currently connected to your IRC network. Arguments,"),
  N_("if given, can be used to restrict the output result."),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /squery",
  "",
};

static usage_t set_usage = {
  N_("usage: /set [[setting] [value]]"),
  "",
  N_("Alter Swirc settings."),
  "",
  (TXT_BOLD "SETTING TYPES" TXT_BOLD),
  "",
  (TXT_BOLD "bool" TXT_BOLD),
  N_("    Bools are case insensitive and can have one of the following"),
  N_("    values:"),
  N_("    - on, true or yes"),
  N_("    - off, false or no"),
  "",
  (TXT_BOLD "int" TXT_BOLD),
  N_("    Integers. Swirc implements a min/max value for each integer in"),
  N_("    order to keep its value safe. The error log will tell if the"),
  N_("    restrictions for an integer aren't within limits and, if so, that"),
  N_("    a preprogrammed fallback value is being used instead."),
  "",
  (TXT_BOLD "string" TXT_BOLD),
  N_("    An arbitrary sequence of characters"),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Output current values of all settings:"),
  N_("    /set (without any arguments)"),
  "",
  N_("  Turn beeps on/off:"),
  "    /set beeps on",
  "    /set beeps off",
  "",
};

static usage_t squery_usage = {
  N_("usage: /squery <servicename> <text>"),
  "",
  N_("This command is used similarly to '/msg'. The only difference is that"),
  N_("the recipient MUST be a service."),
  "",
};

static usage_t theme_usage = {
  N_("usage: /theme [install <name> | list-remote | set <name>]"),
  "",
  N_("Management of themes on-the-fly."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("    Install a theme named 'bx':"),
  "        /theme install bx",
  "",
  N_("    List all available themes:"),
  "        /theme list-remote",
  "",
  N_("    Activate an installed theme with name 'bx':"),
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
