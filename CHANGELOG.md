# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Added ###
- Event 435. Undocumented in the RFC. (Cannot change nickname while
  banned on channel.)
- Event 437 `ERR_UNAVAILRESOURCE`
- Option -i for ICB mode (Internet Citizen's Band). Planning to
  support the protocol.
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
- On event join/part handle an empty user/host.
- In `event_notice`: show prefix/params on error.
- The prompt for the status window to nothing.
- In event_privmsg(): handle an empty user/host.
- Connection timeout to 15 seconds.
- Don't verify peer per default on WIN32 on SSL connections due to
  unable to get local issuer certificate.
- Decrease `NAMES_HASH_TABLE_SIZE` to 4500.
- In `event_topic_chg`: handle an empty user/host.
- In `event_kick`: handle an empty user/host.

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
- In readline.c in function session_destroy(): get rid of possibly
  issued warning regarding: discards qualifiers from pointer target
  type.
- Duplicated code.

## [1.1] - 2016-08-27 ##
### Added ###
- Window scrolling capabilities!
- On OpenBSD: restrict system operations by using pledge() if it's
  available.
- Command /banlist
- Event 367 `RPL_BANLIST`
- Event 368 `RPL_ENDOFBANLIST`
- Event 324 `RPL_CHANNELMODEIS`
- Event 329. Undocumented. Channel date of creation.
- Command /chanserv
- Command /nickserv
- Event 487. Undocumented in the RFC. Prints out an error saying that
  /msg target ChanServ/NickServ is no longer supported (in favor of
  /chanserv and /nickserv).
- Command /exlist
- Command /ilist
- Event 346-349 (numeric replies to /exlist and /ilist).

### Changed ###
- Usage for command /list was bogus.
- Document keys pg up/down in the manual page.
- Decorated output of /list a bit.
- In the interpreter: handle string truncation when copying an
  identifier/argument.
- `textbuffer_size_absolute` now defaults to 1500.
- Only call `unget_wch(MY_KEY_RESIZE)` after sleeping for a requested
  interval. This possibly prevents the program from crashing or the
  terminal from being hangup due to a "resize attack".
- In command /msg: fail if recipient is ChanServ or NickServ. Use of
  the commands /chanserv and /nickserv should be considered instead.
- In the printtext module: replace unbounded `sw_sprintf` calls with
  `sw_snprintf`.

### Fixed ###
- An issue where the cursor was left at the statusbar (after updating
  it).
- In `net_ssl_send`: signed-unsigned mix with relational.
- An issue regarding the maintenance of channel statistics.

## [1.0] - 2016-08-13 ##
### Added ###
- Support for multiple encodings: UTF-8, ISO-8859-1 and ISO-8859-15.
- Command /list
- Event 321-323 (numeric replies for /list).
- Event 412 `ERR_NOTEXTTOSEND`
- Event 481 `ERR_NOPRIVILEGES`
- Event 341 `RPL_INVITING`
- Event 742 (MODE cannot be set due to channel having an active MLOCK
  restriction policy). ATM not documented in the RFC.
- Event 334. Undocumented. At Undernet, it prints out usage describing
  parameters for the LIST command.
- Event 467 `ERR_KEYSET`
- Event 471-474

### Changed ###
- The default theme. Color codes that consisted of only one digit were
  changed to use two digits in some cases. For example: ^C3 changed to
  ^C03, ^C5 changed to ^C05, etc. Why? Consider a function call like
  printtext(..., "%s`one or more digits`", Theme("primary_color")). If
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
