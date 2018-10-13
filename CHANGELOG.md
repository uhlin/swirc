# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Added ###
- A `swirc.conf` manual page
- Option
  - `account_notify`

### Deleted ###
- Option `encoding`
- Option `recode`

### News ###
- Windows-specific:
  - Passes tests of Windows App Certification Kit

## [2.2] - 2018-09-30 ##
### Added ###
- Option `ircv3_server_time` (currently a no-op)
- Option `nickname_aliases`. A space separated list of nickname
  aliases.
- Several code improvements
- Windows 10 toast notifications
- Unit tests for
  - `is_alphabetic()`
  - `is_numeric()`

### Changed ###
- Certain error messages
- Output Swirc homepage on CTCP version reply

### Fixed ###
- Some functions that should be static, but weren't...
- Possibly lossy conversions

### News ###
- Windows-specific:
  - MSI installer

## [2.1] - 2018-05-21 ##
### Added ###
- New build system for
  - UNIX
  - Windows
- Unit tests for
  - `int_diff`
  - `int_sum`
  - `size_product`
  - `squeeze_text_deco()`
  - `sw_strcat()`
  - `sw_strcpy()`
- Use of `size_product` in multiple places
- Command /set

### Changed ###
- Name of function
  - `Strcolor` to `strColor`
  - `Strdup_printf` to `strdup_printf`
  - `Strdup_vprintf` to `strdup_vprintf`
  - `Strfeed` to `strFeed`
  - `Strings_match_ignore_case` to `strings_match_ignore_case`
  - `Strings_match` to `strings_match`
- Linux-specific: fix cflags with pkg-config
- Send `/whois` on `spawn_chat_window()` when on air and the window
  isn't an irc channel
- The printtext color map

### Fixed ###
- Building for NetBSD

### News ###
- Windows-specific:
  - Curl 7.60.0
  - LibreSSL 2.7.3
  - PDCurses 3.6

## [2.0] - 2018-02-24 ##
### Added ###
- Event 465 `ERR_YOUREBANNEDCREEP`
- Event 466 `ERR_YOUWILLBEBANNED`
- Command alias /cs (chanserv)
- Command alias /ns (nickserv)
- `xfopen()` which is a wrapper for `fopen` and `fopen_s`
- `xmbstowcs()` which is a wrapper for `mbstowcs` and `mbstowcs_s`
- `xstrerror()` which is a wrapper for `strerror_s` and `strerror_r`
- `size_to_int()` which asserts given conversation isn't lossy

### Changed ###
- Windows-specific: always produce UTF-8 output

### Fixed ###
- Errors in the license (more specifically in the disclaimer)

## [1.9] - 2017-11-12 ##
### Added ###
- Key CTRL+a (move to beginning of line)
- Key CTRL+e (move to end of line)

### Changed ###
- Windows-specific: machine type from x86 to x64 which means that a
  64-bit executable will be built next time

### Fixed ###
- Windows-specific: library issues causing the newly introduced
  command /sasl to work improperly

## [1.8] - 2017-10-27 ##
### Added ###
- Event 486 (You must log in with services to message this user)
- Option `sasl`
- Option `sasl_mechanism`
- Option `sasl_password`
- Option `sasl_username`
- Event 462 `ERR_ALREADYREGISTRED`
- Event 901 `RPL_LOGGEDOUT`
- Event 902 `ERR_NICKLOCKED`
- Event 903 `RPL_SASLSUCCESS`
- Event 904 `ERR_SASLFAIL`
- Event 905 `ERR_SASLTOOLONG`
- Event 906 `ERR_SASLABORTED`
- Event 907 `ERR_SASLALREADY`
- Tor hidden service to freenode servers
- Command /sasl
- Support for SASL authentication during registration process
- Key F11 (close window)
- Key F12 (close all private conversations)
- Many small improvements

### Fixed ###
- Browsing the command history caused the program to freeze if the
  next/prev command exceeded a certain count

## [1.7] - 2017-08-12 ##
### Added ###
- Enhancements to the printtext module
- Theme item `slogan`
- Command /theme for management of themes on-the-fly
- WIN32: use of function `MessageBox()` in the errHand module
- Event 442 `ERR_NOTONCHANNEL`

### Changed ###
- Theme item `sw_ascLogotype_color` to `logo_color`
- WIN32: theme item `term_use_default_colors` will always be set to NO
- Turned `nodelay()` OFF which reduces CPU usage

### Fixed ###
- Buggy behaviour of `squeeze_text_deco`

## [1.6] - 2017-05-24 ##
### Added ###
- Event 479. Illegal channel name.
- Event 716, 717. Error responses for CTCP requests.

### Changed ###
- The statusbar to display channel modes

### Fixed ###
- Using plain integer as NULL pointer
- Possible dereference of NULL pointer
- A couple of small errors
- Handle if server sends names for a particular channel twice or
  more. See this as a vulnerability fix.
- A memory leak in `net_ssl_check_hostname`. The leak occurred each
  time a new TLS/SSL connection was established if config option
  `hostname_checking` was set to YES (the default).

## [1.5] - 2017-03-05 ##
### Added ###
- Event 461 `ERR_NEEDMOREPARAMS`
- Function `X509_check_host` for implementations that lacks support
  for it
- Config option `cipher_suite`. Valid values are secure, compat,
  legacy and insecure
- Improved TLS/SSL security
- Via command /connect it's now possible to connect, only by
  specifying a particular irc network, that is: "efnet", "freenode",
  "ircnet", "quakenet" or "undernet"
- Config option `hostname_checking`

### Changed ###
- Connection timeout to 45 seconds
- Set `ssl_verify_peer` to YES even on WIN32
- ASCII logo
- Title for status window (to Swirc homepage)
- Statusbar to display user modes

### Fixed ###
- Usage for command /who was bogus. Parameter mask is mandatory.
- Key enter for Windows.
- Unget `\n` right after connection to a server. May or may not fix
  junk characters from sometimes appearing at the prompt.
- Command history bug fix.

## [1.4] - 2017-02-03 ##
### Added ###
- WIN32 specific: added a resource script
- Broadcast channel activity if our nickname is highlighted
- Command /version (CTCP)
- Support for reading a CTCP VERSION reply requested by /version
- Command /time (CTCP)
- Support for reading a CTCP TIME reply requested by /time
- Command /close
- Event 492. Official name: `ERR_NOSERVICEHOST`. On InspIRCd: User
  does not accept CTCPs
- Event 500. Undocumented in the RFC. (Only a server may modify the +r
  user mode)

### Changed ###
- If target port is 6697: set SSL on
- In `event_notice`: handle an empty user/host

### Fixed ###
- Reduced code duplication
- A bogus behaviour in command /connect
- CTCP VERSION reply bug
- Save nickname, username and real name automatically to the config if
  customized by the command-line
- Better performance

## [1.3] - 2017-01-14 ##
### Added ###
- Event 435. Undocumented in the RFC. (Cannot change nickname while
  banned on channel)
- Event 437 `ERR_UNAVAILRESOURCE`
- Option -i for ICB mode (Internet Citizen's Band). Planning to
  support the protocol
- Command /who
- Event 352 `RPL_WHOREPLY`
- Event 315 `RPL_ENDOFWHO`
- Event 477 `ERR_NOCHANMODES`
- Event 421 `ERR_UNKNOWNCOMMAND`
- Event 444 `ERR_NOLOGIN`
- Broadcast window activity for private messages
- Command /rules
- Event 232, 308 and 309 (numeric replies for /rules -- undocumented
  in the RFC)
- Command /cycle
- Event 484 `ERR_RESTRICTED`

### Changed ###
- On event join/part handle an empty user/host
- In `event_notice`: show prefix/params on error
- The prompt for the status window to nothing
- In `event_privmsg`: handle an empty user/host
- Connection timeout to 15 seconds
- Don't verify peer per default on WIN32 on SSL connections due to
  unable to get local issuer certificate
- Decrease `NAMES_HASH_TABLE_SIZE` to 4500
- In `event_topic_chg`: handle an empty user/host
- In `event_kick`: handle an empty user/host
- In `event_quit`: handle an empty user/host

### Fixed ###
- Signal 11 when connecting to [Slack](https://slack.com/).
- Instead of using slots for chunks of names, use a linked list.
- In the printtext module: switch off all terminal attributes during
  indentation.

## [1.2] - 2016-09-22 ##
### Added ###
- Checks in order to prohibit the program from running with superuser
  privileges (aka root).
- Checks for printf-like functions.
- In strHand.c: documentation.
- In printtext.c: documentation.
- Command history!
- In a config or theme: log if a fallback value is chosen for a
  certain setting. This means that the setting contains an invalid
  value.
- Support for really large channels.
- Function to show error log size on startup.

### Changed ###
- The name of function `PrintAndFree` to `print_and_free`.
- In `print_and_free`: print to the active window.
- Rewrote commands to use `print_and_free`.
- For OS X and GNU/Linux: compile using optimization level 2.
- Option -p. Instead of taking an argument it will now ask for a
  password upon connect if it's specified.
- In `event_names_print_all`: adjust the column-width with respect to
  the longest name in each column.

### Fixed ###
- A bug regarding that mode +q has different meanings on different
  server software (ircds).
- Possible crash due to resizing the terminal while connection is in
  progress.
- In the printtext module: a problem with converting a wide character
  to a multibyte sequence due to EILSEQ.

## [1.1+] - 2016-08-31 ##
### Changed ###
- Increase `NAMES_HASH_TABLE_SIZE` to 6500.

### Fixed ###
- In `src/unix.mk`: provide a standard default target "all".
- In the manual page: AUTHORS section without An macro.
- In io-loop.c: get rid of zero-length printf format string
  warnings. They weren't harmful in the first place.
- In readline.c in function `session_destroy`: get rid of possibly
  issued warning regarding: discards qualifiers from pointer target
  type.
- Duplicated code.

## [1.1] - 2016-08-27 ##
### Added ###
- Window scrolling capabilities!
- On OpenBSD: restrict system operations by using pledge() if it's
  available
- Command /banlist
- Event 367 `RPL_BANLIST`
- Event 368 `RPL_ENDOFBANLIST`
- Event 324 `RPL_CHANNELMODEIS`
- Event 329. Undocumented. Channel date of creation
- Command /chanserv
- Command /nickserv
- Event 487. Undocumented in the RFC. Prints out an error saying that
  /msg target ChanServ/NickServ is no longer supported (in favor of
  /chanserv and /nickserv)
- Command /exlist
- Command /ilist
- Event 346-349 (numeric replies to /exlist and /ilist)

### Changed ###
- Usage for command /list was bogus
- Document keys pg up/down in the manual page
- Decorated output of /list a bit
- In the interpreter: handle string truncation when copying an
  identifier/argument
- `textbuffer_size_absolute` now defaults to 1500
- Only call `unget_wch(MY_KEY_RESIZE)` after sleeping for a requested
  interval. This possibly prevents the program from crashing or the
  terminal from being hangup due to a "resize attack"
- In command /msg: fail if recipient is ChanServ or NickServ. Use of
  the commands /chanserv and /nickserv should be considered instead
- In the printtext module: replace unbounded `sw_sprintf` calls with
  `sw_snprintf`

### Fixed ###
- An issue where the cursor was left at the statusbar (after updating
  it).
- In `net_ssl_send`: signed-unsigned mix with relational.
- An issue regarding the maintenance of channel statistics.

## [1.0] - 2016-08-13 ##
### Added ###
- Support for multiple encodings: UTF-8, ISO-8859-1 and ISO-8859-15
- Command /list
- Event 321-323 (numeric replies for /list)
- Event 412 `ERR_NOTEXTTOSEND`
- Event 481 `ERR_NOPRIVILEGES`
- Event 341 `RPL_INVITING`
- Event 742 (MODE cannot be set due to channel having an active MLOCK
  restriction policy). ATM not documented in the RFC
- Event 334. Undocumented. At Undernet, it prints out usage describing
  parameters for the LIST command
- Event 467 `ERR_KEYSET`
- Event 471-474

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
