#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2023 Markus Uhlin
# SPDX-License-Identifier: ISC

if [ "$(whoami)" = "root" ]; then
	echo "do not run as root"
	exit 1
fi

check_tools()
{
	local _tools="gcab"

	for _tool in ${_tools}; do
		printf "checking for %s..." "${_tool}"
		if [ -x "$(/bin/which ${_tool})" ]; then
			echo "found"
		else
			echo "fatal: not found"
			exit 1
		fi
	done
}

check_tools

CAB_DATE="$(date +%Y%m%d)"
FOLDER="swirc-locales-${CAB_DATE}"
MO_NAME="swirc.mo"

test -d "${FOLDER}" && rm -frv "${FOLDER}"
test -f "${FOLDER}.cab" && rm -fv "${FOLDER}.cab"

mkdir -p "${FOLDER}/de/LC_MESSAGES" || exit 1
mkdir -p "${FOLDER}/fi/LC_MESSAGES" || exit 1
mkdir -p "${FOLDER}/fr/LC_MESSAGES" || exit 1
mkdir -p "${FOLDER}/sv/LC_MESSAGES" || exit 1

CP_FLAGS="-fv"

test -f "de/${MO_NAME}" && \
    cp ${CP_FLAGS} de/${MO_NAME} "${FOLDER}/de/LC_MESSAGES/"
test -f "fi/${MO_NAME}" && \
    cp ${CP_FLAGS} fi/${MO_NAME} "${FOLDER}/fi/LC_MESSAGES/"
test -f "fr/${MO_NAME}" && \
    cp ${CP_FLAGS} fr/${MO_NAME} "${FOLDER}/fr/LC_MESSAGES/"
test -f "sv/${MO_NAME}" && \
    cp ${CP_FLAGS} sv/${MO_NAME} "${FOLDER}/sv/LC_MESSAGES/"

gcab -cv "${FOLDER}.cab" "${FOLDER}"
test -f "${FOLDER}.cab" && echo "cab successfully created"
