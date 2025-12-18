# Check whether timingsafe_memcmp() exists
#
# SPDX-FileCopyrightText: Copyright 2025 Markus Uhlin
# SPDX-License-Identifier: BSD-3-Clause

check_timingsafe_memcmp () {
	local _tmpfile _srcfile _out

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <string.h>

int
main(void)
{
	char array1[] = {'a','b','c'};
	char array2[] = {'a','b','c'};
	int res;

	res = timingsafe_memcmp(array1, array2, 3);
	return (res == 0 ? 0 : 1);
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	printf "checking for timingsafe_memcmp()..."

	${CC} ${CFLAGS} -Werror "$_srcfile" -o "$_out" ${LDFLAGS} \
	    >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_TIMINGSAFE_MEMCMP=1
EOF
	else
		echo "no"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_TIMINGSAFE_MEMCMP=0
EOF
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
