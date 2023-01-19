#!/usr/bin/env bash

if [ "$(whoami)" = "root" ]; then
	echo "running the program as root is forbidden"
	exit 1
fi

DICT_FILE=irc.dict
NEXECS=943718
TMPFILE=$(mktemp log.XXXXXXXXXX)

afl-fuzz -i "in" -o "out" -n -x "${DICT_FILE}" -E "${NEXECS}" -- \
    ./append-log "${TMPFILE}"
