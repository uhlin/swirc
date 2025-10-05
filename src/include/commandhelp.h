#ifndef COMMAND_HELP_H
#define COMMAND_HELP_H
/* commandhelp.h
   Copyright (C) 2018-2025 Markus Uhlin. All rights reserved.

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

static usage_t admin_usage = {
  N_("usage: /admin [target]"),
  "",
  N_("The admin command is used to find information about the administrator\n"
     "of the given server, or current server if 'target' parameter is omit-\n"
     "ted."),
  "",
};

static usage_t announce_usage = {
  N_("usage:"),
  "    /announce [doit|list|new|rm] [args]",
  "    /announce doit <#>",
  "    /announce list",
  "    /announce new <[yes|no]> <chan1[,chan2][...]> <msg>",
  "    /announce rm <[#|all]>",
  "",
  N_("Send announcements on IRC."),
  "",
};

static usage_t away_usage = {
  N_("usage: /away [reason]"),
  "",
  N_("Marks yourself as being away (with a reason). If the reason is omitted\n"
     "you will be marked as no longer being away."),
  "",
};

static usage_t ban_usage = {
  N_("usage: /ban <mask>"),
  "",
  N_("Sets a channel ban which rejects all users whose 'nick!user@host'\n"
     "matches the provided mask from joining the channel. Wildcards are okay\n"
     "and the active window must be an IRC channel."),
  "",
};

static usage_t banlist_usage = {
  N_("usage: /banlist [channel]"),
  "",
  N_("Outputs a channel's banlist. If the channel argument is left empty and\n"
     "the active window is an IRC channel, Swirc will output the banlist for\n"
     "that channel."),
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
  N_("Communicate with your IRC network's channel service. If the initial\n"
     "argument equals to '--', then the value of setting 'chanserv_host' is\n"
     "used as a service hostname."),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /nickserv",
  "",
};

static usage_t ctcp_usage = {
  N_("usage: /ctcp <query> <target>"),
  "",
  N_("Send a CTCP query to 'target', which is either a nickname or an IRC\n"
     "channel."),
  "",
  N_("Query can be either:"),
  "  - time",
  "  - userinfo",
  "  - version",
  "",
};

static usage_t cap_usage = {
  N_("usage: /cap [ls | list]"),
  "",
  N_("Lists the (IRCv3) capabilities supported by the server and/or the\n"
     "capabilities associated with the active connection."),
  "",
};

static usage_t cleartoasts_usage = {
  N_("usage: /cleartoasts"),
  "",
  N_("On Windows Swirc sends toast notifications. By running this command\n"
     "all notifications associated with Swirc will be cleared."),
  "",
};

static usage_t close_usage = {
  N_("usage: /close"),
  "",
  N_("Closes the active window. It's not possible to close the status\n"
     "window. And while connected it's not possible to close a channel, in\n"
     "that case instead use '/part'."),
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
  N_("If the port is omitted port 6667 will be chosen. And if the port is\n"
     "7326 ICB mode is turned on automatically. Further, if the port is 6697\n"
     "Swirc attempts to initiate a TLS/SSL connection, as well as if '-tls'\n"
     "is entered."),
  "",
  N_("It is possible to connect to a certain IRC network by only entering\n"
     "the network name. For example: '/connect -tls libera', will connect to\n"
     "Libera Chat using an encrypted connection. Preprogrammed network names\n"
     "are:"),
  "",
  "    - afternet",
  "    - alphachat",
  "    - anonops",
  "    - efnet",
  "    - freenode",
  "    - ircnet",
  "    - ircnow",
  "    - libera",
  "    - oftc",
  "    - quakenet",
  "    - undernet",
  "",
};

static usage_t cycle_usage = {
  N_("usage: /cycle [channel]"),
  "",
  N_("Cycle a channel, i.e. '/part' plus '/join'. If the channel argument is\n"
     "omitted and the active window is an IRC channel, the client will cycle\n"
     "that channel."),
  "",
};

static usage_t dcc_usage = {
  N_("usage:"),
  "    /dcc [clear|get|list|ls|send] [args]",
  "    /dcc clear [get|send|completed|all]",
  "    /dcc get <nick> <file>",
  "    /dcc list [get|send|all]",
  "    /dcc ls [up|down]",
  "    /dcc send <nick> <file>",
  "",
  N_("Get and send files. Swirc implements its own variant of DCC meaning\n"
     "it's incompatible with other IRC clients. Transport Layer Security is\n"
     "forced and for now the DCC feature isn't available in ICB mode."),
  "",
};

static usage_t deop_usage = {
  N_("usage: /deop <nick>"),
  "",
  N_("Remove the channel operator privilege from another user."),
  "",
};

static usage_t devoice_usage = {
  N_("usage: /devoice <nick>"),
  "",
  N_("Remove the channel voice privilege from another user."),
  "",
};

static usage_t die_usage = {
  N_("usage: /die [--I-am-sure]"),
  "",
  N_("An IRC operator can use this command to shutdown the server. Please\n"
     "confirm that this is what you really want by typing '--I-am-sure'."),
  "",
};

static usage_t disconnect_usage = {
  N_("usage: /disconnect [message]"),
  "",
  N_("Disconnect from IRC, but don't quit the program. A disconnect message\n"
     "is optional."),
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
  N_("Outputs a channel's exception list. An exception mask (+e) overrides a\n"
     "ban mask. If the channel argument is omitted and the active window is\n"
     "an IRC channel, the client will output the exception list for that\n"
     "channel."),
  "",
};

static usage_t fetchdic_usage = {
  N_("usage: /fetchdic [name]"),
  "",
  N_("Fetches spelling dictionaries. If the name argument is omitted Swirc\n"
     "will output a list of available dictionaries. The list is obtained\n"
     "remotely which means its contents can be updated at any time."),
  "",
  (TXT_BOLD "EXAMPLE" TXT_BOLD),
  "",
  N_("Fetch the american english dictionary:"),
  "    /fetchdic en_US",
  "",
};

static usage_t ftp_usage = {
  N_("usage:"),
  "    /ftp cd <path>",
  "    /ftp del <path>",
  "    /ftp exit",
  "    /ftp get <file>",
  "    /ftp login",
  "    /ftp ls [dir|up|down]",
  "    /ftp mkdir <path>",
  "    /ftp pwd",
  "    /ftp rmdir <path>",
  "    /ftp send <file>",
  "    /ftp system",
  "",
  N_("Retrieve and store files on a FTP server. (The communication is done\n"
     "in clear text, i.e. unencrypted.)"),
  "",
};

static usage_t gline_usage = {
  N_("usage: /gline <nick!user@host> [<duration> :<reason>]"),
  "",
  N_("Network-wide bans."),
  "",
  N_("When a client matches a G-line it cannot connect to ANY server on the\n"
     "IRC network for 'duration' seconds. If the duration is zero then the\n"
     "G-line will be permanent."),
  "",
  N_("(If no duration and no reason is given the G-line is deleted.)"),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /kline",
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
  N_("Outputs a list of all available commands."),
  N_("(Or help for a specific command.)"),
  "",
};

static usage_t ignore_usage = {
  N_("usage: /ignore [regex]"),
  "",
  N_("Ignores all 'nick!user@host' that matches the given regular\n"
     "expression, this by using the POSIX basic regular expression\n"
     "grammar. This command isn't to be used by beginners and I advice you\n"
     "to be careful when using it. I highly recommend the use of:"),
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
  N_("Outputs a channel's invitation list. An invitation mask (+I) overrides\n"
     "the invite-only flag (+i). If the channel argument is omitted and the\n"
     "active window is an IRC channel, the client will output the invitation\n"
     "list for that channel."),
  "",
};

static usage_t info_usage = {
  N_("usage: /info [target]"),
  "",
  N_("The info command returns information about the server."),
  "",
};

static usage_t invite_usage = {
  N_("usage: /invite <targ_nick> <channel>"),
  "",
  N_("Invites 'targ_nick' to a channel."),
  "",
};

static usage_t ison_usage = {
  N_("usage: /ison <nick1> [nick2] [nick3] [...]"),
  "",
  N_("Checks whether users are on IRC."),
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
  N_("Kicks one or more users out of a channel. The users are given in a\n"
     "comma-separated list. A reason is optional and the active window must\n"
     "be an IRC channel."),
  "",
};

static usage_t kickban_usage = {
  N_("usage: /kickban <nick> <mask> [reason]"),
  "",
  N_("Set a channel ban with given 'mask' and kick the user 'nick' out of a\n"
     "channel. A reason is optional and the active window must be an IRC\n"
     "channel."),
  "",
};

static usage_t kill_usage = {
  N_("usage: /kill <nickname> <comment>"),
  "",
  N_("Disconnect a user from the connected network."),
  N_("(Requires IRC op privilege.)"),
  "",
};

static usage_t kline_usage = {
  N_("usage: /kline <nick!user@host> [<duration> :<reason>]"),
  "",
  N_("Server-local bans."),
  "",
  N_("When a client matches a K-line it cannot connect to the local server\n"
     "for 'duration' seconds. If the duration is zero then the K-line will\n"
     "be permanent."),
  "",
  N_("(If no duration and no reason is given the K-line is deleted.)"),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /gline",
  "",
};

static usage_t list_usage = {
  N_("usage: /list [<max_users[,>min_users][,pattern][...]]"),
  "",
  N_("Lists channels and their topics. Without any arguments the list is\n"
     "HUGE. For example, '/list >1500' will only list channels that have\n"
     "more than 1500 users."),
  "",
  N_("Depending on the IRC server software used by your network the usage\n"
     "may differ."),
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
  N_("    I - set/remove an invitation mask to override the invite-only flag"),
  "",
  (TXT_BOLD "USER MODES" TXT_BOLD),
  "",
  N_("    i - marks a user as invisible"),
  N_("    w - the user receives wallops messages"),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("    Give channel operator privilege to 'Companion' in #foo:"),
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
  N_("Used to send private messages between users, as well as to send\n"
     "messages to channels."),
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
  N_("If the initial argument equals to '--' then the:"),
  "",
  N_("    1) Value of setting 'nickserv_host' will be used as service"),
  N_("       hostname."),
  N_("    2) Command call won't be added to the command history provided"),
  N_("       that the second argument is 'identify'."),
  "",
  N_("The correct service hostname is not always the same as the visible\n"
     "hostname of NickServ. FYI at the AnonOps IRC network the visible\n"
     "hostname of NickServ is anonops.in (when this text was written)\n"
     "but you should use 'services.anonops.com'. As a fallback:"),
  N_("'/query NickServ' can be used in order to communicate with the\n"
     "service."),
  "",
};

static usage_t notice_usage = {
  N_("usage: /notice <recipient> <message>"),
  "",
  N_("Used to send private messages between users, as well as to send\n"
     "messages to channels. (In notice form.)"),
  "",
};

static usage_t nsid_usage = {
  N_("usage: /nsid [--force] [--swap] [--pass-only]"),
  "",
  N_("NickServ identify."),
  "",
  N_("The username and password used for the identification are read from\n"
     "'sasl_username' and 'sasl_password' respectively. And the service\n"
     "hostname from 'nickserv_host'."),
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
  N_("Leaves a channel (optionally with a message). If the command is called\n"
     "without any arguments and the current window is an IRC channel, that\n"
     "channel will be the target."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Leave channel #chatzone with message 'bye':"),
  N_("    /part #chatzone bye"),
  "",
};

static usage_t passmod_usage = {
  N_("usage: /passmod <nickname>"),
  "",
  N_("Pass the ICB moderation privilege to another group member."),
  "",
};

static usage_t qbot_usage = {
  N_("usage: /qbot <[service hostname | --]> <command> [...]"),
  "",
  N_("Communicate with QuakeNet's network service, the Q bot. If the initial\n"
     "argument equals to '--', then the value of setting 'qbot_host' is used\n"
     "as a service hostname."),
  "",
};

static usage_t query_usage = {
  N_("usage: /query [nick]"),
  "",
  N_("Starts a private conversation with 'nick'. If 'nick' is omitted and\n"
     "the active window is a private conversation, the action will be to\n"
     "close it."),
  "",
};

static usage_t quit_usage = {
  N_("usage: /quit [message]"),
  "",
  N_("Disconnect from IRC and quit the program. A disconnect message is\n"
     "optional."),
  "",
};

static usage_t rehash_usage = {
  N_("usage: /rehash"),
  "",
  N_("The rehash command is an administrative command which can be used by\n"
     "an IRC operator to force the server to re-read and process its conf-\n"
     "iguration file."),
  "",
};

static usage_t resize_usage = {
  N_("usage: /resize"),
  "",
  N_("Resize the terminal. For example, Windows doesn't send 'SIGWINCH',\n"
     "instead this command can be used. First resize the window then run\n"
     "this command."),
  "",
};

static usage_t restart_usage = {
  N_("usage: /restart [--I-am-sure]"),
  "",
  N_("An IRC operator can use the 'restart'-command to force the server to\n"
     "restart itself."),
  "",
};

static usage_t rules_usage = {
  N_("usage: /rules"),
  "",
  N_("Outputs network/server rules. Not all IRC server software supports\n"
     "this command. (It's actually quite rare.)"),
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
  "    mechanism [ecdsa-nist256p-challenge | external | plain |",
  "               scram-sha-1,256,512]",
  "    username <name>",
  "    password <pass>",
  "    passwd_s <sasl pass> <encryption pass>",
  "    set [on | off]",
  "    x509 <filename>",
  "",
  N_("SASL is a method that lets you identify with NickServ during the\n"
     "connection process eliminating the need to do it manually."),
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
  N_("  (The only IRC network that I know of that is supporting this\n"
     "  mechanism is Libera Chat.)"),
  "",
  N_("  Identification using mechanism 'plain':"),
  "    1) /sasl mechanism plain",
  "    2) /sasl username <your nickserv username>",
  "    3) /sasl password <your nickserv password>",
  "    4) /sasl set on",
  "",
};

static usage_t say_usage = {
  N_("usage: /say <message>"),
  "",
  N_("Say a message. This command can be used if you want say something with\n"
     "a leading command-character, i.e. a slash."),
  "",
  (TXT_BOLD "EXAMPLE" TXT_BOLD),
  "",
  N_("    /say // A single-line comment in C++"),
  "",
};

static usage_t servlist_usage = {
  N_("usage: /servlist [<mask> [<type>]]"),
  "",
  N_("Lists services currently connected to your IRC network. Arguments,\n"
     "if given, can be used to restrict the output result."),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /squery",
  "",
};

static usage_t servstats_usage = {
  N_("usage: /servstats [<query> [<target>]]"),
  "",
  N_("This command is used to query statistics of a certain IRC server."),
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
  N_("    Bools are case insensitive and can have one of the following\n"
     "    values:"),
  N_("    - on, true or yes"),
  N_("    - off, false or no"),
  "",
  (TXT_BOLD "int" TXT_BOLD),
  N_("    Integers. Swirc implements a min/max value for each integer in\n"
     "    order to keep its value safe. The error log will tell if the\n"
     "    restrictions for an integer aren't within limits and, if so, that\n"
     "    a preprogrammed fallback value is being used instead."),
  "",
  (TXT_BOLD "string" TXT_BOLD),
  N_("    An arbitrary sequence of characters"),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Output the current values of all settings:"),
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
  N_("This command is used similarly to '/msg'. The only difference is that\n"
     "the recipient MUST be a service."),
  "",
};

static usage_t stats_usage = {
  N_("usage: /stats [channel]"),
  "",
  N_("Outputs a channel's user statistics. If the channel argument is left\n"
     "empty and the active window is an IRC channel, Swirc will output the\n"
     "user statistics for that channel."),
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
  N_("usage: /time <target>"),
  "",
  N_("Send a CTCP TIME request to 'target', which is either a nickname or an\n"
     "IRC channel."),
  "",
};

static usage_t topic_usage = {
  N_("usage: /topic [new topic]"),
  "",
  N_("Sets a new topic for a channel. If 'new topic' is omitted the action\n"
     "will be to display the current topic. (The active window must be an\n"
     "IRC channel.)"),
  "",
};

static usage_t unban_usage = {
  N_("usage: /unban <mask>"),
  "",
  N_("Removes a channel ban. (The active window must be an IRC channel.)"),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /ban",
  "    /banlist",
  "",
};

static usage_t unignore_usage = {
  N_("usage: /unignore [#]"),
  "",
  N_("Deletes a regular expression from the ignore list."),
  "",
  (TXT_BOLD "SEE ALSO" TXT_BOLD),
  "",
  "    /ignore",
  "",
};

static usage_t utctime_usage = {
  N_("usage: /utctime"),
  "",
  N_("Prints out the coordinated universal time (UTC)."),
  "",
};

static usage_t version_usage = {
  N_("usage: /version <target>"),
  "",
  N_("Send a CTCP VERSION request to 'target', which is either a nickname or\n"
     "an IRC channel."),
  "",
};

static usage_t voice_usage = {
  N_("usage: /voice <nick>"),
  "",
  N_("Gives the channel voice privilege to another user."),
  "",
};

static usage_t wallops_usage = {
  N_("usage: /wallops <message>"),
  "",
  N_("The 'wallops'-command is used to send a message to all currently\n"
     "connected users who have set the 'w' user mode for themselves."),
  "",
};

static usage_t who_usage = {
  N_("usage: /who <mask>"),
  "",
  N_("Generates a query which returns a list of information which matches\n"
     "the provided 'mask'."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Show the Libera Chat crew:"),
  "    /who libera/staff/*",
  "",
  N_("  Show users with a German domain name:"),
  "    /who *.de",
  "",
};

static usage_t whois_usage = {
  N_("usage: /whois <nick>"),
  "",
  N_("Asks after information about another user."),
  "",
};

static usage_t wholeft_usage = {
  N_("usage: /wholeft"),
  "",
  N_("By using this command you can see who left during a network server\n"
     "split. The command takes no arguments and the active window must be an\n"
     "IRC channel."),
  "",
};

static usage_t znc_usage = {
  N_("usage: /znc [*module] <command>"),
  "",
  N_("Simplifies the communication with ZNC which is a popular\n"
     "'IRC bouncer'."),
  "",
  (TXT_BOLD "EXAMPLES" TXT_BOLD),
  "",
  N_("  Output your ZNC version:"),
  "    /znc version",
  "",
  N_("  Identical to the previous example:"),
  "    /znc *status version",
  "",
};

//lint +e786

#endif
