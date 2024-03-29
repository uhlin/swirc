# link_with_gnu_libidn.sh:
# Link with GNU libidn
#
# SPDX-FileCopyrightText: Copyright 2021-2023 Markus Uhlin
# SPDX-License-Identifier: BSD-3-Clause

link_with_gnu_libidn () {
	local _tmpfile _srcfile _out _libs

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	_libs="-lidn"
	cat <<EOF >"$_srcfile"
#include <idn-free.h>
#include <stdio.h>
#include <stringprep.h>

int
main(void)
{
	const char *cp;

	idn_free(NULL);
	if ((cp = stringprep_check_version(NULL)) != NULL)
		puts(cp);
	return 0;
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	printf "checking for gnu libidn..."

	${CC} ${CFLAGS} -Werror "$_srcfile" -o "$_out" ${LDFLAGS} ${_libs} \
	    >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_LIBIDN=1
LDLIBS += ${_libs}
EOF
	else
		echo "no"
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
