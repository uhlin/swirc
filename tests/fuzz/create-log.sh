#!/usr/bin/env bash

if [ "$(whoami)" = "root" ]; then
	echo "running the program as root is forbidden"
	exit 1
elif [ $# -ne 1 ]; then
	echo "bogus number of args"
	exit 1
fi

INCLUDE_COMMON=$1
shift

if [ "${INCLUDE_COMMON}" != "y" ] && [ "${INCLUDE_COMMON}" != "n" ]; then
	echo "bad argument"
	exit 1
fi

DICT_FILE=irc.dict
NEXECS=471859
TMPFILE=$(mktemp log.XXXXXXXXXX)

if [ "${INCLUDE_COMMON}" = "y" ]; then
	cat <<EOF >"${TMPFILE}"
:localhost 001 Mo :Welcome
:localhost 002 Mo :Your host is 127.0.0.1
:localhost 003 Mo :This server was created ...
:localhost 004 Mo localhost 1.0 abc def
:localhost 005 Mo FOO BAR
:localhost 005 Mo BAZ QUX :are supported by this server
:localhost 042 Mo XXXXXXXXX :your unique ID
:localhost 251 Mo :There are 1 users and 0 services on 1 servers
:localhost 252 Mo 0 :operators online
:localhost 253 Mo 0 :unknown connections
:localhost 254 Mo 0 :channels formed
:localhost 255 Mo :I have ...
:localhost 265 Mo 0 0 :Current local users ...
:localhost 266 Mo 0 0 :Current global users ...
:localhost 375 Mo :- 127.0.0.1 Message of the Day -
:localhost 372 Mo :...
:localhost 376 Mo :End of MOTD command.
EOF
fi

afl-fuzz -i "in" -o "out" -n -x "${DICT_FILE}" -E "${NEXECS}" -- \
    ./append-log "${TMPFILE}"
