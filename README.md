# README #

![Swirc Logo](https://www.nifty-networks.net/swirc/gfx/swirc-royal-110x110.png)

## What is Swirc? ##

Swirc is a BSD licensed, console based and lightweight ICB and IRC
client written in C/C++, whose goals are to be portable and secure.

## Program options ##

### Common ###

    -c <server[:port]>    Auto-connect to given server
    -n <nickname>         Online nickname
    -u <username>         User identity
    -r <rl name>          Specifies the real name
    -i                    ICB mode
    -p                    Server password (for private servers)
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
* [Ncurses](https://www.gnu.org/software/ncurses/ncurses.html)
  with wide character support
* [OpenSSL toolkit](https://www.openssl.org/)

Which means that on for example a Debian GNU/Linux system you need to
install 3 packages before building:

    # apt install libcurl4-openssl-dev libncursesw5-dev libssl-dev

And on Mac OS X, provided that
[Homebrew](http://brew.sh/)
is installed, issue:

    $ brew install libressl

#### NetBSD ####

    # pkgin install ncursesw

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

Installing it (under `/usr/local`):

    $ sudo make install

### Building for Windows ###

To build Swirc for Windows you must have
[Visual Studio](http://www.visualstudio.com/).

So, fire up the command prompt for
[Visual Studio](http://www.visualstudio.com/)
where the needed tools (the compiler, etc.) are loaded into the
environment. The regular command prompt won't work. Then:

    > cd c:\path\to\swirc
    > nmake -f Makefile.vc

*Done!*

To make a distribution of Swirc use:

    > nmake -f Makefile.vc dist

## Cleaning ##

Examples:

    $ make clean
    $ nmake -f Makefile.vc clean
