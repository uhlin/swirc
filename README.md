# README #

## What is Swirc? ##

Swirc is a text-mode IRC client whose goals are to be portable, and to
provide a secure & user-friendly experience to its users.  IRC stands
for Internet Relay Chat, and is a protocol for text communication in
real time over the Internet.

The name Swirc is an assembly of the word Swift (or at your option:
Swedish) and IRC.

Swirc is not mIRC. [mIRC](https://www.mirc.com/) is an IRC client,
like Swirc is.
They both handles the
[IRC client protocol](https://raw.githubusercontent.com/uhlin/swirc/master/doc/rfc2812.txt).
Multiple different IRC clients exists, just like that there are
multiple different web browsers. Chrome, Firefox, IE to name a
few. Google "server and client". By clarifying this I hope less
confusion will occur.

### Extensions ###

I have no plans to add support for writing extensions in a scripting
language. Extensions are added to the repository written in C, if they
can make it there.

## Program options ##

### Common ###

    -c <server[:port]>    Auto-connect to given server
    -n <nickname>         Online nickname
    -u <username>         User identity
    -r <rl name>          Specifies the real name
    -i                    ICB mode
    -p                    Upon connect: ask for a password
    -h <hostname>         Use this hostname on connect
    -x <config>           Explicit config file

### Extras ###

    -v, -version          View the current client ver
    -?, -help             Print the usage

## Cloning ##

To clone the repository use [Git](https://git-scm.com).

    $ git clone https://github.com/uhlin/swirc.git

## Building ##

### Framework ###

Swirc currently depends on:

* [Curl](https://curl.haxx.se/libcurl/)
* [Ncurses](https://www.gnu.org/software/ncurses/ncurses.html) with wide character support
* [OpenSSL toolkit](https://www.openssl.org/)

Which means that on for example a Debian GNU/Linux system you need to
install 3 packages before building:

    # aptitude install libcurl4-gnutls-dev:amd64 libncursesw5-dev libssl-dev

And on Mac OS X, provided that [Homebrew](http://brew.sh/) is
installed, issue:

    $ brew install libressl

### Building for the UNIX environment ###

On the BSDs and GNU/Linux the configure script will per default
generate make definitions that expects that the C compiler
[GCC](https://gcc.gnu.org/) is installed on your system.
A make utility must also be present. Regarding Mac OS X I suggest that
you install [Xcode](https://developer.apple.com/xcode/).
Due to certain circumstances I no longer can confirm that building for
OS X works.

    $ ./configure
    $ cd src
    $ make -f unix.mk

Installing it (under `/usr/local`):

    $ sudo make -f unix.mk install

### Building for Windows ###

To build Swirc for Windows you must have [Visual Studio](http://www.visualstudio.com/).

So, fire up the command prompt for [Visual Studio](http://www.visualstudio.com/)
where the needed tools (the compiler, etc.) are loaded into the
environment. The regular command prompt won't work. Then:

    > cd c:\path\to\swirc\src
    > nmake -f w32.mk

Done! The executable will be put in the `src` directory.

## Cleaning ##

Examples:

    $ make -f unix.mk clean
    $ nmake -f w32.mk clean
