# README #

![Swirc Logo](https://www.nifty-networks.net/swirc/gfx/swirc-royal-110x110.png)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/18215/badge.svg)](https://scan.coverity.com/projects/uhlin-swirc)
[![CodeQL](https://github.com/uhlin/swirc/actions/workflows/codeql.yml/badge.svg)](https://github.com/uhlin/swirc/actions/workflows/codeql.yml)
[![macOS](https://github.com/uhlin/swirc/actions/workflows/macos.yml/badge.svg)](https://github.com/uhlin/swirc/actions/workflows/macos.yml)

## What is Swirc? ##

Swirc is a BSD licensed, console based and lightweight ICB and IRC
client written in C/C++, whose goals are to be portable and secure.

[Official Swirc homepage](https://www.nifty-networks.net/swirc/)

## Program options ##

usage: swirc [-46?CPRSXdipv] [-W password] [-c server[:port]] [-j join] [-n nickname] [-r rl name] [-u username] [-x config]

    -4                   Use IPv4 addresses only
    -6                   Use IPv6 addresses only
    -?, --help           Output help
    -C                   Do not change color definitions
    -P                   Permanently disable SASL authentication
    -R                   Disable TLS/SSL peer verification
    -S                   Force TLS
    -W <password>        Equal effect as flag 'p' but non-interactive
    -X                   Disable all IRCv3 extensions
    -c <server[:port]>   Connect to IRC server
    -d                   Debug logging
    -i                   Turn on Internet Citizen's Band mode
    -j <join>            A comma-separated list of channels to join
    -n <nickname>        Online nickname
    -p                   Query for server password (for private servers)
    -r <rl name>         Your real name
    -u <username>        Your username
    -v, --version        Output Swirc version
    -x <config>          Config file

## SAST Tools ##

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Cloning ##

To clone the repository use [Git](https://git-scm.com).

    $ git clone https://github.com/uhlin/swirc.git

## Building ##

### Framework ###

Swirc currently depends on:

* [Curl](https://curl.haxx.se/libcurl/)
* [GNOME libnotify](https://wiki.gnome.org/)
* [GNU gettext](https://www.gnu.org/software/gettext/)
* [GNU libiconv](https://www.gnu.org/software/libiconv/)
* [GNU libidn](https://www.gnu.org/software/libidn/)
* [Hunspell](https://hunspell.github.io/)
* [Ncurses](https://www.gnu.org/software/ncurses/ncurses.html)
  with wide character support
* [OpenSSL toolkit](https://www.openssl.org/)

Which means that on for example a Debian GNU/Linux system you need to
install these packages before building:

    # apt install gettext libcurl4-openssl-dev libhunspell-dev libidn11-dev libncursesw5-dev libssl-dev

And on Mac OS X, provided that
[Homebrew](http://brew.sh/)
is installed, issue:

    $ brew install hunspell
    $ brew install libressl

Prompts that begin with a hash(#) symbolizes that the command shall be
executed as root, while prompts that begin with a dollar sign ($)
symbolizes that the command shall be executed as a normal user.

#### Void Linux ####

    # xbps-install -S gettext-devel hunspell-devel libcurl-devel libidn-devel libnotify-devel ncurses-devel openssl-devel

### Building for the UNIX environment ###

On the BSDs and GNU/Linux the configure script will per default
generate make definitions that expects that the C compiler
[GCC](https://gcc.gnu.org/)
is installed on your system. A make utility must also be
present. Regarding Mac OS X I suggest that you install
[Xcode](https://developer.apple.com/xcode/).
Due to certain circumstances I no longer can confirm that building for
OS X works.

    $ cd /path/to/swirc
    $ ./configure
    $ make

#### Configuration options ####

The following options can be passed to the configure script:

- `--with-libnotify`: Enable support for desktop notifications
- `--without-libiconv`: Build without GNU libiconv
- `--without-libidn`: Build without GNU libidn
- `--without-libintl`: No internationalization

#### Install ####

1. Installing it under `/usr/local`:

        $ sudo make install

2. Installing it under `/home/user` without the translations (in which
   case you also should've passed `--without-libintl` to the configure
   script):

        $ PREFIX=/home/user make install-no-lc-msgs

### Building for Windows ###

To build Swirc for Windows you must have
[Visual Studio](http://www.visualstudio.com/).

So, fire up the command prompt for
[Visual Studio](http://www.visualstudio.com/)
where the needed tools (the compiler, etc.) are loaded into the
environment. The regular command prompt won't work. Then:

    cd c:\path\to\swirc
    nmake -f Makefile.vc

*Done!*

To make a distribution of Swirc use:

    nmake -f Makefile.vc dist

## Cleaning ##

Examples:

    $ make clean
    $ nmake -f Makefile.vc clean
