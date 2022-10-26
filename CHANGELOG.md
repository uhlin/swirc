# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
- Added and made use of `printf_and_free()`
- Added the following events:
  - 413 (`ERR_NOTOPLEVEL`)
  - 414 (`ERR_WILDTOPLEVEL`)
  - 415 (`ERR_BADMASK`)
- Added usage of `__func__`

## [3.3.7] - 2022-10-22 ##
- **Added** code to the following scripts:
  - `posixshell/link_with_gnu_libidn.sh`
  - `posixshell/link_with_libnotify.sh`
- **Added** preprogrammed IRC network [OFTC](https://www.oftc.net/)
- **Added** usage of `swircpaths.h`, a header which is automatically
  generated during the build process.
- **Fixed** a case of a possibly negative array subscript

### Windows ###
- **Fixed** a bug that made Swirc fail to start with error `renamed
  executable`. This was due to that `_get_pgmptr()` signaled success
  but stored an empty string, which only occurred on some Windows
  setups. (Reported by [cpkio](https://github.com/cpkio) - thanks!)

## [3.3.6] - 2022-10-05 ##
- **Added** cryptographic functions
- **Added** subcommand `passwd_s` to `/sasl` which can be used to
  securely store encrypted SASL passwords in `swirc.conf`. The
  encryption technique practice the use of **AES** and **SHA**.
- **Added** tab completion for
  - `/msg`
  - `/notice`
  - `/sasl`
- **Changed** the program behavior to
  - jump between ICB and IRC mode depending on the port number
  - **not** create core dump files if `NDEBUG` is defined at compile time
  - **not** echo the server password on input (command line flag `-p`)
- **Fixed** a bug so if a connection attempt fails it's possible to
  reconnect again after `/disconnect`.
- **Fixed** code duplication
- **Fixed** insufficient out of range check, off-by-one, in command
  `/unignore`.
- Made small improvements
  - **Added** usage of C++17 fallthrough attr
  - **Added** usage of `PATH_MAX`
  - **Added** usage of `__func__` instead of hardcoding it (for example in
    error messages)
  - **Fixed** redundant checks
  - Reduced scope of variables
  - Switched to usage of 'sizeof' in `BZERO()` calls
  - ...
- Prevented the config hash table from being paged to the swap area.
- **Rewrote** `commands/sasl.c` in C++
- **Updated** preprogrammed IRC servers
  - **Added** port numbers and server descriptions
  - **Added** the [IRCNow](https://ircnow.org/) network
  - **Deleted** the [Blitzed](http://blitzed.org/) network

## [3.3.5] - 2022-07-28 ##
- **Added** command `/cap`
- **Added** config option `iconv_conversion` (bool)
- **Added** config option `mouse_events`
- **Added** new translations
- **Added** usage of `noexcept` and installed a terminate handler
- **Changed** the startup screen to show the current language
- **Fixed** C style casts in C++
- **Fixed** SASLprep by switching to usage of `stringprep_profile()`. Prep
  now works on Windows too!
- **Fixed** memory leaks
- **Fixed** unhandled exceptions
- Improved memory handling
- Improved the contents of `/help`
  - Completed translating everything to Swedish
- **Made** config option `mouse` change take affect at once
- **Made** improvements to `events/cap.c`
- **Made** small optimizations
- **Made** the dot mo files available for read operations with `unveil()`
- Updated the TODO :-)

### Windows ###
- Added a GNU bundle containing:
  1. [gettext runtime](https://www.gnu.org/software/gettext/) 0.21
  2. [libiconv](https://www.gnu.org/software/libiconv/) 1.17
  3. [libidn](https://www.gnu.org/software/libidn/) 1.38
- Fixed missing DLL file `libiconv-2.dll`. I had it in my PATH so I
  didn't notice it.

## [3.3.4] - 2022-05-04 ##
- **Added** a TLS server (to be used in a future version)
- **Added** config option `mouse` (defaults to **no**)
- **Added** config option `server_cipher_suite`
- **Added** creation of OpenSSL scripts
- **Added** event 335 (`RPL_WHOISBOT`)
- **Added** global hashfunctions
- **Added** preprogrammed IRC network
  [AlphaChat](https://www.alphachat.net/)
- **Added** scrolling using the mouse
- **Added** theme item:
  - `nicklist_my_nick_color`
  - `whois_bot`
- **Added** translations
- **Changed** the statusbar to display readline stats
- **Converted**
  - `commands/services.c` -> `commands/services.cpp`
  - `config.c` -> `config.cpp`
- **Defined** `SLASH` once
- **Defined** and made use of:
  - `UNUSED_PARAM()`
  - `UNUSED_VAR()`
  - `colorarray_t`
  - `g_beginthread_failed`
  - `g_received_welcome`
  - `usage_t`
- **Fixed** bugs discovered with protocol fuzzing
- Handle PRIVMSGs from "my" server
- Handle truncation of ICB messages, i.e. allow longer messages than
  `ICB_MESSAGE_MAX`.
- Improved scrolling
- Made refactoring to multiple files
- Moved the names hash table modify API
- Print PRIVMSGs to irc channels from users that aren't in them. (No
  lookup error.)
- Reduced code duplication
- Reformatted and reindented files

### Windows ###
- **Added** GNU LibIntl 0.19.8.1 which is part of
  [gettext](https://www.gnu.org/software/gettext/)
- [Curl](https://curl.se/) 7.83.0
- [LibreSSL](https://www.libressl.org/) 3.5.2

## [3.3.3] - 2021-11-27 ##
- **Added** event `531`. Undocumented in the RFC. (You cannot send
  CTCPs to this user whilst they have the +T `u_noctcp` mode set.)
- **Added** event `728` and `729`. Undocumented in the RFC. Channel
  Quiet List.
- **Added** function `xstrnlen()` and made use of it
- **Added** new [Libera Chat](https://libera.chat/) servers
- **Added** null checks
- **Deleted** unused includes
- **Fixed** macro redefinitions
- **Fixed** unchecked return values
- Made improvements to the following files:
  - `commands/invite.c`
  - `commands/notice.cpp`
  - `commands/services.c`
  - `events/invite.cpp`
  - `events/wallops.cpp`
  - `assertAPI.c`
  - `errHand.c`
  - `icb.c`
    - Added status message `Idle-Mod`.
    - Added status message `Timeout`.
  - `readlineTabCompletion.c`
    - Refactoring
  - ...
- Reformatted and reindented files:
  - `commands/me.c`
  - `commands/msg.c`
  - `commands/squery.cpp`
  - `commands/znc.cpp`
  - `events/account.cpp`
  - `events/banlist.cpp`
  - `events/servlist.cpp`
  - `statusbar.cpp`
  - `titlebar.c`
  - ...

### Debian ###
- Added patches

### Windows ###
- Upgraded to [LibreSSL](https://www.libressl.org/) 3.4.2

## [3.3.2] - 2021-10-17 ##
- **Added** command line option `-j`
- **Added** function `getuser()` and made use of it
- **Added** null checks
- **Added** the following command aliases:
  - `/j` (`/join`)
  - `/p` (`/part`)
- **Added** the following commands:
  - `/ignore`
  - `/unignore`
- **Fixed** unchecked return values
- Made improvements to the following files:
  - `commands/connect.c`
  - `events/channel.cpp`
  - `events/misc.cpp`
  - `events/notice.cpp`
  - `events/privmsg.cpp`
  - `events/welcome.cpp`
  - `events/whois.cpp`
  - `dataClassify.c`
  - `log.c`
  - `nestHome.c`
- Reformatted and reindented files
- **Rewrote** the `/join` and `/part` commands in C++
- **Updated** the translations

### Windows ###
- **Fixed** terminal resizing. (Increasing the size is ok.)
- The [PDC](https://pdcurses.org/) library has been rebuilt with the
  option `UTF8=Y` which makes PDCurses ignore the system locale. So
  for Windows computers running Swirc UTF-8 is now forced. This means
  you should enable the option **Use Unicode UTF-8 for worldwide
  language support**. In order to do so under Windows 10:
  1. Language preferences
  2. Administrative language settings
  3. Change system locale
  4. Check the option and click OK

## [3.3.1] - 2021-09-03 ##
- **Added** command line option `-R` (Disable TLS/SSL peer verification)
- **Added** configuration flag `--without-libintl`
- **Added** key F1 (Output help)
- Configuration option `joins_parts_quits` now defaults to **NO**
- Documented F5 - F10 in command `/colormap`
- **Fixed** "no text to send" bug
- Made improvements to the following files:
  - `events/auth.c`
- Translated parts to **German**, **Finnish**, **French** and
  **Swedish**.

## [3.3.0] - 2021-06-27 ##
- **Added** function `errdesc_by_last_err()` and made use of it
- **Added** key F3 (scroll nicklist up) and F4 (scroll nicklist down)
- **Added** new modes and options for TLS/SSL connections
- **Added** nicklist
- **Added** null checks
- **Added** preprogrammed network name [libera](https://libera.chat/)
- **Added** support for partial writes in `net_ssl_send()`
- **Added** theme item `nicklist_nick_color`
- **Added** theme item `nicklist_privilege_color`
- **Added** theme item `nicklist_vline_color`
- Asserted that the program is terminated correctly
- **Defined** and made use of `addrof()`
- **Defined** and made use of `g_textdeco_chars`
- Deleted `ToastActivator_i.c`
- Deleted `ToastActivator_p.c`
- Deleted `dlldata.c`
- Deleted command `/n`
- Explicitly set client mode for TLS/SSL connections
- **Fixed** "use after free" bug in `/cycle`
- **Fixed** a bug in `/quit` that resulted in SIGPIPE due to calling
  `SSL_shutdown()` on an already shutdown socket.
- **Fixed** the behavior of `net_ssl_recv()` by checking the condition of
  `SSL_pending()`.
- **Fixed** unchecked return values
- Made improvements to the following files:
  - `io-loop.c`
  - `irc.c`
  - `network-openssl.c`
  - `network.cpp`
  - `pthrMutex.c`
  - `readline.c`
  - `readlineAPI.c`
  - `sig-unix.c`
  - `sig-w32.c`
  - `vcMutex.c`
  - `window.c`
- Modified the scrolling behavior
- Moved defines
- Reformatted and reindented files
- Renamed functions and patterns
- **Rewrote the printtext module in C++ and made multiple improvements**!
- Upgraded to:
  - [Curl](https://curl.se/) 7.77.0
  - [LibreSSL](https://www.libressl.org/) 3.3.3

## [3.2.7] - 2021-03-13 ##
- Added checking of `term_is_too_small()`
- Added null checks
- Defined `g_conversion_failed` and made use of it
- Defined `g_time_error` and made use of it
- Fixed a bug that could lead to null pointer comparison
- Fixed limited prompt length
- Fixed stricter checking of channel names
- Fixed unchecked return values and made code improvements
- Improved automatic resizing
- Reduced code duplication
- Rewrote `event_invite()` and thus fixed a vulnerability that imply
  that a malicious IRC server message could cause a crash. (Reported
  by Michael Ortmann.)
- Rewrote `events/names.c` using C++ and made various improvements

## [3.2.6] - 2021-02-17 ##
- Added logging of program name and PID for debug/error messages
- Added tab completion for:
  - `/help`
  - `/znc`
- Fixed multiple non-ANSI function declarations
- Fixed multiple sign-compare warnings
- Fixed unchecked return values
- Improved the configure script
- Splitted the configure script into smaller parts
- Made functions that yet weren't declared at file scope to be
- WIN32: fixed behavior in `/quit`
- WIN32: upgraded to [LibreSSL](https://www.libressl.org/) 3.2.4

## [3.2.5] - 2020-10-09 ##
### Added ###
- Command
  - `/servlist`: used to list services currently connected to the
    network.
  - `/squery`: used for communication with irc network services.
  - `/znc` (for communication with [znc](https://www.znc.in/))
- Event
  - 234 (`RPL_SERVLIST`)
  - 235 (`RPL_SERVLISTEND`)
  - 493\. Undocumented in the RFC. ngIRCd: `You must share a common
    channel with [...]`.

### Changed ###
- `network-openssl.c`: `suite_secure`:
  `TLSv1.3:TLSv1.2+AEAD+ECDHE:TLSv1.2+AEAD+DHE`
- From obsolete `ctime()` calls to instead use `strftime()`
- To seed the random number generator using a new approach. (The
  deterministic RNG isn't used in a security context anyway.)

### Fixed ###
- Command-line option `-P`. (It disabled SASL authentication but
  didn't end IRCv3 capability negotiation.)

### News ###
- [LibreSSL](https://www.libressl.org/) 3.1.4

## [3.2.4] - 2020-04-08 ##
### Added ###
- Improved documentation
- More colors for Windows and a synchronized colormap
- Make target (`check`) for running unittests
- Command-line option
  - `-C` (Do not change color definitions)
  - `-P` (Permanently disable SASL authentication)
- Tab completion for
  - `/query`
  - `/whois`

### Changed ###
- Display logging in statusbar

### Fixed ###
- A possible out-of-bounds read/write array operation

## [3.2.3] - 2020-03-22 ##
### Added ###
- Code improvements

### Changed ###
- Multiple events to use try-catch blocks
- Stopped using `free_not_null()` as `free()` handles null

### Fixed ###
- Color 11

## [3.2.2] - 2020-03-17 ##
### Added ###
- **Multicolor support**. For terminals supporting minimum 256 colors.
- Command
  - `/colormap`
  - `/echo`
- Tab completion for `/set`

### Fixed ###
- Color (Note: For terminals supporting minimum 16 colors)
  - Grey
  - Light Grey
- Multiple occurrences where:
  - Pointer parameters could be declared as pointing to const
- Tab completion bugs

## [3.2.1] - 2020-03-01 ##
### Added ###
- **Code improvements**
- Fetching of the Mozilla CA certificate store (in PEM format) for
  Windows builds
- Improved help and documentation
- Updated setver script :-)

### Fixed ###
- Multiple occurrences where:
  - Pointer parameters could be declared as pointing to const
  - Previously assigned value to a variable hasn't been used

## [3.2.0] - 2020-02-13 ##
### Added ###
- **Logging**. Which can be toggled on/off with CTRL+L per window and
  works while IRC connected. The default is off for all windows.
- **Tab completion**
- Better detection for connection loss

### Changed ###
- For C++, prefer `const_cast` and `static_cast` respectively, instead
  of C style casts.

### Fixed ###
- Possibly lossy type conversions
- Readline bugs

## [3.1.1] - 2019-12-22 ##
### Fixed ###
- Possible readline deadlock

### News ###
- [Curl](https://curl.haxx.se/) 7.67.0
- [LibreSSL](https://www.libressl.org/) 3.0.2

## [3.1.0] - 2019-12-06 ##
### Added ###
- Command-line option
  - `-4` (Use IPv4 addresses)
  - `-6` (Use IPv6 addresses)
  - `-d` Debug logging
- Event
  - 464 (`ERR_PASSWDMISMATCH`)
- Preprogrammed network name:
  - [afternet](https://www.afternet.org/)
  - [blitzed](http://blitzed.org/)
- SASL auth mechanism [SCRAM-SHA-256](https://tools.ietf.org/html/rfc7677)

### Changed ###
- Made command `/say` available for ICB

### Fixed ###
- Command-line option `-p` (which queries the user for a connection
  password). It has been broken for some time but I haven't noticed it
  until now.
- Connection to a IPv6-only host. (Reported by Ross Richardson).
- Reconnection with connection password

### Updated ###
- Help for command
  - `/connect`
  - `/sasl`

## [3.0.0] - 2019-10-31 ##
### Added ###
- **SUPPORT FOR THE ICB PROTOCOL**
- Command
  - `/beep`
  - `/boot`
  - `/group`
  - `/kill`
  - `/passmod`

### Changed ###
- Error messages (in order to improve them)

### Fixed ###
- Better reconnection
- Improved functions
- Multiple occurrences of `Conditional expression should have
  essentially Boolean type`
- Spelling errors

## [2.9.0] - 2019-09-10 ##
### Added ###
- **Code improvements**
- Cleaner auto-generation of configs and themes
- More color pairs

### Changed ###
- Tell if a command is unknown
- Tell if away from keyboard

### Deleted ###
- `say()`

### Fixed ###
- Colorized statusbar

### News ###
- [PDCurses](https://pdcurses.org/) 3.9

## [2.8.0] - 2019-07-16 ##
### Added ###
- Command
  - `/ban` + `/unban`
  - `/kickban`
  - `/op` + `/deop`
- Event
  - 696\. Undocumented in the RFC. (You must specify a parameter for
    the key mode).
  - 698\. Undocumented in the RFC. (Channel ban list does not contain
    `...`).
- Preprogrammed network name [anonops](https://www.anonops.com/).

### Changed ###
- `RPL_BOUNCE`->`RPL_ISUPPORT`
- `SW_NORET`->`NORETURN`
- Don't shutdown irc connection on runtime error in:
  - `event_join()`
  - `event_kick()`
  - `event_part()`

### Deleted ###
- `swirc_wprintw()` due to deprecation

### Fixed ###
- Reported by [MAGA](https://github.com/MakeItGreatAgain):
  - Issues with protocol colon escaping
  - Reproducible crash from protocol fuzzing
  - `/cycle` fails when a channel key is in use
- Reproducible crashes from protocol fuzzing

## [2.7.2] - 2019-05-26 ##
### Added ###
- **A rewritten interpreter**
- **Linux**: hardened builds
- Code improvements
- Event
  - 416\. Officially undocumented. (output too large, truncated)
- Improved help for command
  - `/mode`
  - `/theme`
  - `/who`

### Changed ###
- Install-destination of the manual pages

### Fixed ###
- Missing null checks

## [2.7.1] - 2019-04-26 ##
### Added ###
- A banner in WiX

### Changed ###
- Names module to use DJB2 hashing (for more efficient results)

### Fixed ###
- Impact of a possibly uninitialized variable
- Possible resource leaks
- Unchecked return values

### News ###
- [LibreSSL](https://www.libressl.org/) 2.9.1

## [2.7.0] - 2019-04-20 ##
### Added ###
- Command
  - `/cleartoasts`
- Improved help for command
  - `/join`
  - `/set`
- Option
  - `beeps`

### Changed ###
- Configure script messages
- Default values for the following options:
  - `nickname`
  - `real_name`
  - `username`
- UNIX: Produce a stripped executable
- WIN32: Compile with O2

### Deleted ###
- Option
  - `disable_beeps`

### News ###
- [Curl](https://curl.haxx.se/) 7.64.1
- [LibreSSL](https://www.libressl.org/) 2.8.3

## [2.6.1] - 2019-03-30 ##
### Added ###
- Algorithm improvements
- The possibility of desktop notifications with the help of configure
  flag `--with-libnotify`

## [2.6] - 2019-03-21 ##
### Added ###
- Code improvements
- Event
  - **PONG**
  - 451 (`ERR_NOTREGISTERED`)
- Option
  - `joins_parts_quits` (Show JOIN/PART/QUIT events?)
  - `reconnect_backoff_delay`: The number of seconds that should be
    added to each reconnect attempt (0-99)
  - `reconnect_delay`: Seconds to consume before the first reconnect
    attempt (0-999)
  - `reconnect_delay_max`: Maximum reconnect delay in seconds
    (0-999). Regardless of the other related reconnect settings.
  - `reconnect_retries`: If the IRC connection is lost, how many
    attempts should be performed to get the connection working again
    before giving up?

### Changed ###
- **Lowered recv/send timeouts during connection establishment for
  faster processing**
- Don't require irc connection for command `/disconnect`
- Scrolling behavior with the help of `arc4random()`

### Fixed ###
- Bugs in command
  - /kick
  - /part
- Typos

### News ###
- [PDCurses](https://pdcurses.sourceforge.io/) 3.8

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
- `size_to_int()` which asserts given conversion isn't lossy
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
