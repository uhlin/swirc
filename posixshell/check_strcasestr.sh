#############################################################
#
# Check whether strcasestr() exists
#
# Copyright (c) 2022, 2023 Markus Uhlin. All rights reserved.
#

check_strcasestr () {
	local _tmpfile _srcfile _out

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#if (defined(__gnu_hurd__) || defined(__linux__)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <string.h>

int
main(void)
{
	char *str;

	str = strcasestr("swirc IRC client", "irc");
	return (str ? 0 : 1);
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	printf "checking for strcasestr()..."

	${CC} ${CFLAGS} -Werror "$_srcfile" -o "$_out" ${LDFLAGS} \
	    >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_STRCASESTR=1
EOF
	else
		echo "no"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_STRCASESTR=0
EOF
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
