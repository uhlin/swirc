# README #

![SW globe](https://dataswamp.org/~markus/swirc/gfx/sw-globe.png)

## What is Swirc? ##

Swirc is a text-mode IRC client whose goals are to be portable, and to
provide a secure & user-friendly experience to its users.  IRC stands
for Internet Relay Chat, and is a protocol for text communication in
real time over the Internet.

The name Swirc is an assembly of the word swift (or at your option
Swedish) and IRC.

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
    -p <password>         Password (for private networks)
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

* [Ncurses](https://www.gnu.org/software/ncurses/ncurses.html) with wide character support
* [OpenSSL toolkit](https://www.openssl.org)

Which means that on for example a Debian GNU/Linux system you need to
install two packages before building:

    # aptitude install libncursesw5-dev libssl-dev

And on Mac OS X, provided that [Homebrew](http://brew.sh/) is
installed, issue:

    $ brew install libressl

### Building for the UNIX environment ###

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
