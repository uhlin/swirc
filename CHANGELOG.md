# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Fixed ###
- Improved scrolling
- Typos

## [2.5] - 2018-12-21 ##
### Added ###
- Extended help for ALL commands

### Changed ###
- Toast activated launch argument

### Deleted ###
- Windows: `SpawnMessageLoop()` happened to be a *mistake* and was
  therefore deleted!

### Fixed ###
- **Cases of missing deinitializations regarding names in a channel!**
- **Thread termination!**

### News ###
- Readline is now non-blocking during connection to a server!
- Reorganized changelog :-)
- Windows: **Improved installer**

## [2.4] - 2018-11-11 ##
### Added ###
- Event AWAY
- Option
  - `away_notify`
  - `invite_notify`

### Changed ###
- Rewrote
  - `event_cap()`
  - `event_privmsg()`
  - ...
  - **MULTIPLE** functions to use try-catch blocks

### Fixed ###
- **A dependency on uninitialized memory!**
- Memory leaks
- Off-by-one error

### News ###
- Curl 7.62.0
- LibreSSL 2.8.2

## [2.3] - 2018-10-24 ##
### Added ###
- A `swirc.conf` manual page
- Command /oper
- Event
  - `ACCOUNT`
  - `WALLOPS`
  - 381 (`RPL_YOUREOPER`)
  - 491 (`ERR_NOOPERHOST`)
  - 908 (`RPL_SASLMECHS`)
- Option
  - `account_notify`
  - `auto_op_yourself`

### Changed ###
- **Option `ircv3_server_time` is now working**

### Deleted ###
- Option
  - `encoding`
  - `recode`

### News ###
- **OpenBSD**: Uses `unveil()` if available
- **Windows**: Passes tests of Windows App Certification Kit

## [2.2] - 2018-09-30 ##
### Added ###
- Option
  - `ircv3_server_time` **currently a no-op**
  - `nickname_aliases`. A space separated list of nickname aliases
    which are used, in addition to the default nickname, to highlight
    a message if it matches any of the aliases given by this setting.
- Several code improvements
- Unit tests for
  - `is_alphabetic()`
  - `is_numeric()`
- Windows 10 toast notifications

### Changed ###
- Certain error messages
- Output Swirc homepage on CTCP version reply

### Fixed ###
- Possibly lossy conversions
- Some functions that should be static, but weren't...

### News ###
- **Windows**: MSI installer

## [2.1] - 2018-05-21 ##
### Added ###
- Command /set
- New build system for
  - UNIX
  - Windows
- Unit tests for
  - `int_diff()`
  - `int_sum()`
  - `size_product()`
  - `squeeze_text_deco()`
  - `sw_strcat()`
  - `sw_strcpy()`
- Use of `size_product()` in multiple places

### Changed ###
- **Linux**: fix cflags with pkg-config
- Name of function
  - `Strcolor` to `strColor`
  - `Strdup_printf` to `strdup_printf`
  - `Strdup_vprintf` to `strdup_vprintf`
  - `Strfeed` to `strFeed`
  - `Strings_match_ignore_case` to `strings_match_ignore_case`
  - `Strings_match` to `strings_match`
- Send `/whois` on `spawn_chat_window()` when on air and the window
  isn't an irc channel.
- The printtext color map

### Fixed ###
- Building for NetBSD

### News ###
- Curl 7.60.0
- LibreSSL 2.7.3
- PDCurses 3.6

## [2.0] - 2018-02-24 ##
### Added ###
- Command alias
  - /cs (for chanserv)
  - /ns (for nickserv)
- Event
  - 465 (`ERR_YOUREBANNEDCREEP`)
  - 466 (`ERR_YOUWILLBEBANNED`)
- `size_to_int()` which asserts given conversation isn't lossy
- `xfopen()` which is a wrapper for `fopen` and `fopen_s`
- `xmbstowcs()` which is a wrapper for `mbstowcs` and `mbstowcs_s`
- `xstrerror()` which is a wrapper for `strerror_s` and `strerror_r`

### Changed ###
- **Windows**: always produce UTF-8 output

### Fixed ###
- Errors in the license (more specifically in the disclaimer)

## [1.9] - 2017-11-12 ##
### Added ###
- Key
  - CTRL+a (move to beginning of line)
  - CTRL+e (move to end of line)

### Changed ###
- **Windows**: Machine type from x86 to x64 which means that a
  **64-bit executable** will be built next time

### Fixed ###
- **Windows**: Library issues causing the newly introduced command
  /sasl to work improperly

## [1.8] - 2017-10-27 ##
### Added ###
- Command /sasl
- Event
  - 462 (`ERR_ALREADYREGISTRED`)
  - 486 (You must log in with services to message this user)
  - 901 (`RPL_LOGGEDOUT`)
  - 902 (`ERR_NICKLOCKED`)
  - 903 (`RPL_SASLSUCCESS`)
  - 904 (`ERR_SASLFAIL`)
  - 905 (`ERR_SASLTOOLONG`)
  - 906 (`ERR_SASLABORTED`)
  - 907 (`ERR_SASLALREADY`)
- Key
  - F11 (close window)
  - F12 (close all private conversations)
- Many small improvements
- Option
  - `sasl`
  - `sasl_mechanism`
  - `sasl_password`
  - `sasl_username`
- Support for SASL authentication during registration process
- Tor hidden service to freenode servers

### Fixed ###
- **Browsing the command history caused the program to freeze if the
  next/prev command exceeded a certain count!**

## [1.7] - 2017-08-12 ##
### Added ###
- Command /theme for management of themes on-the-fly
- Enhancements to the printtext module
- Event 442 `ERR_NOTONCHANNEL`
- Theme item `slogan`
- WIN32: use of function `MessageBox()` in the errHand module

### Changed ###
- Theme item `sw_ascLogotype_color` to `logo_color`
- Turned `nodelay()` OFF which reduces CPU usage
- WIN32: theme item `term_use_default_colors` will always be set to NO

### Fixed ###
- **Buggy behavior of `squeeze_text_deco()`!**

## [1.6] - 2017-05-24 ##
### Added ###
- Event
  - 479 (Illegal channel name)
  - 716 and 717 (Error responses for CTCP requests)

### Changed ###
- The statusbar to display channel modes

### Fixed ###
- Occurrences of
  - possible dereferences of null pointers
  - usage plain integers as null pointers
- **Handle if server sends names for a particular channel twice or
  more**
- `net_ssl_check_hostname`: memory leak.

## [1.5] - 2017-03-05 ##
### Added ###
- Config option `cipher_suite`. Valid values are
  - **secure**
  - **compat**
  - **legacy**
  - **insecure**
- Config option `hostname_checking`
- Event 461 (`ERR_NEEDMOREPARAMS`)
- Function `X509_check_host` for implementations that lacks support
  for it
- Via /connect it's now possible to connect, only by specifying a
  particular IRC network, preprogrammed networks are:
  - [efnet](http://www.efnet.org/)
  - [freenode](https://freenode.net/)
  - [ircnet](http://www.ircnet.org/)
  - [quakenet](https://www.quakenet.org/)
  - [undernet](http://www.undernet.org/)

### Changed ###
- ASCII logo
- Connection timeout to 45 seconds
- Set `ssl_verify_peer` to **YES** even on WIN32
- Statusbar to display user modes
- Title for status window (to Swirc homepage)

### Fixed ###
- A command history feature bug
- Improved TLS/SSL security
- Key enter for Windows
- Usage for /who. (Parameter mask is mandatory.)

## [1.4] - 2017-02-03 ##
### Added ###
- Broadcasting of channel activity if our nickname matches certain
  patterns
- Capability of reading a...
  - CTCP TIME reply (requested by /time)
  - CTCP VERSION reply (requested by /version)
- Command
  - /close
  - /time
  - /version
- Event
  - 492\. **Official name**: `ERR_NOSERVICEHOST`. **On InspIRCd**:
    User does not accept CTCPs.
  - 500\. Undocumented in the RFC. (Only a server may modify the +r
    user mode)
- WIN32: A resource script

### Changed ###
- If server port is 6697 automatically set TLS/SSL on

### Deleted ###
- Code duplication

### Fixed ###
- A bogus behavior in /connect
- Better performance
- CTCP VERSION reply bug
- Save nickname, username and real name automatically to the config if
  customized by the command-line.
- `event_notice`: handle an empty *user@host* combination

## [1.3] - 2017-01-14 ##
### Added ###
- Broadcasting of window activity for private messages
- Command
  - /cycle
  - /rules
  - /who
- Command line option `-i` for ICB mode (Internet Citizen's Band).
  Plans are to support the protocol but it's **not implemented yet**!
- Event
  - 232, 308 and 309 (numeric replies for /rules -- undocumented in
    the RFC)
  - 315 (`RPL_ENDOFWHO`)
  - 352 (`RPL_WHOREPLY`)
  - 421 (`ERR_UNKNOWNCOMMAND`)
  - 435\. Undocumented in the RFC. (Cannot change nickname while
    banned on channel).
  - 437 (`ERR_UNAVAILRESOURCE`)
  - 444 (`ERR_NOLOGIN`)
  - 477 (`ERR_NOCHANMODES`)
  - 484 (`ERR_RESTRICTED`)

### Changed ###
- **Attention**: Don't verify peer per default on WIN32 on SSL
  connections due to unable to get local issuer certificate.
- Connection timeout to 15 seconds
- The prompt for the status window to nothing...
- `NAMES_HASH_TABLE_SIZE` to 4500
- `event_notice`: output IRC prefix + params on error.

### Fixed ###
- **Instead of using slots for chunks of names** use a linked list!
- **Printtext**: switch off all terminal attributes during indentation.
- Handle empty *user@host* combination for
  - `event_join()`
  - `event_kick()`
  - `event_part()`
  - `event_privmsg()`
  - `event_quit()`
  - `event_topic_chg()`
- Signal 11 when connecting to [Slack](https://slack.com/).

## [1.2] - 2016-09-22 ##
### Added ###
- A function to show error log size on startup
- Checks for printflike functions
- Checks in order to prohibit the program from running with superuser
  privileges (aka root)
- Command history feature
- Logging to the error log for settings that are detected with invalid
  values

### Changed ###
- **GNU/Linux and OS X**: compile using optimization level 2.
- Command line option `-p`. Instead of taking an argument it will now
  query the user for a password during connection to an IRC server.
- The name of function `PrintAndFree()` to `print_and_free()` and its
  behavior. Plus multiple commands to use it.
- `event_names_print_all()`: adjust the column-width with respect to
  the longest name in each column.

### Fixed ###
- **Printtext**: A problem with converting a wide character to a
  multibyte sequence due to EILSEQ
- A bug regarding that mode +q has different meanings on different
  server software (IRCd's)
- Possible crash due to resizing the terminal while connection is in
  progress

## [1.1+] - 2016-08-31 ##
### Changed ###
- `NAMES_HASH_TABLE_SIZE` to 6500

### Deleted ###
- Duplicated code

### Fixed ###
- Absence of *An* macro in the man page
- Readline: `session_destroy`: Get rid of possibly issued warning
  regarding: discards qualifiers from pointer target type.
- `src/unix.mk`: provide a standard default target **all**
- io-loop: Get rid of zero-length printf format string warnings. (They
  weren't harmful in the first place.)

## [1.1] - 2016-08-27 ##
### Added ###
- Command
  - /banlist
  - /chanserv
  - /exlist
  - /ilist
  - /nickserv
- Additional data to the manual page
- Event
  - 324 (`RPL_CHANNELMODEIS`)
  - 329\. Undocumented. Channel date of creation.
  - 346-349 (numeric replies to /exlist and /ilist)
  - 367 (`RPL_BANLIST`)
  - 368 (`RPL_ENDOFBANLIST`)
  - 487\. Undocumented in the RFC. An error meaning that /msg target
    *ChanServ* or *NickServ* is no longer supported (in favor of
    **/chanserv** and **/nickserv**).
- On OpenBSD: restrict system operations by using pledge() if it's
  available.
- Window scrolling capabilities!

### Changed ###
- **/msg**: fail if recipient is *ChanServ* or *NickServ*. (Use of the
  commands **/chanserv** and **/nickserv** should be considered
  instead.)
- Interpreter: handle string truncation when copying an identifier or
  argument
- Look of the output produced by /list
- Only call `unget_wch(MY_KEY_RESIZE)` after sleeping for a requested
  interval. This possibly prevents the program from crashing or the
  terminal from being hung up due to a "resize attack".
- Printtext: replaced unbounded `sw_sprintf` calls with `sw_snprintf`
- `textbuffer_size_absolute` now defaults to 1500

### Fixed ###
- An issue
  - ...regarding the maintenance of channel statistics
  - ...where the cursor was left at the statusbar (after updating it)
- Visible usage of /list
- `net_ssl_send`: signed-unsigned mix with relational

## [1.0] - 2016-08-13 ##
### Added ###
- Support for multiple encodings:
  - UTF-8
  - ISO-8859-1
  - ISO-8859-15
- Command /list
- Event
  - 321-323 (numeric replies for /list)
  - 334\. Undocumented. At Undernet, it prints out usage describing
    parameters for the LIST command.
  - 341 (`RPL_INVITING`)
  - 412 (`ERR_NOTEXTTOSEND`)
  - 467 (`ERR_KEYSET`)
  - 471-474
  - 481 (`ERR_NOPRIVILEGES`)
  - 742 (MODE cannot be set due to channel having an active MLOCK
    restriction policy). ATM not documented in the RFC.

### Changed ###
- The default theme. Color codes that consisted of only one digit were
  changed to use two digits in some cases. For example: ^C3 changed to
  ^C03, ^C5 changed to ^C05, etc. Why? Consider a function call like
  printtext(..., "%s`one or more digits`", Theme(`primary_color`)). If
  the primary color, in this case, consists of only one digit and is
  directly followed by `one or more digits` the result is: one digit
  is lost and a random color will be displayed.

### Fixed ###
- As a fallback, instead of printing out "Printtext Internal Error",
  continue to convert characters even if some are lost due to an
  invalid multibyte sequence.
- Interpretation of color codes in the printtext module. For example:
  ^C123 didn't display '3' before as it should.

## [1.0b] - 2016-07-30 ##
- FIRST OFFICIAL VERSION OF SWIRC!
