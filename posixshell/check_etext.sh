# Check whether the etext segment is usable
#
# SPDX-FileCopyrightText: Copyright 2023 Markus Uhlin
# SPDX-License-Identifier: BSD-3-Clause

check_etext () {
	local _tmpfile _srcfile _out
	local _val=1

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <stddef.h>

int global;

static int
valid(const void *ptr)
{
	extern char etext;
	return (ptr != NULL && ((const char *) ptr) > &etext);
}

int
main(void)
{
	int local;

	if (valid(&local) && valid(&global))
		return 99;
	return 1;
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	printf "checking whether the etext segment is usable..."

	${CC} ${CFLAGS} -Werror "$_srcfile" -o "$_out" ${LDFLAGS} \
	    >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		eval "$_out"
		_val=$?
	fi

	if [ ${_val} -eq 99 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CPPFLAGS += -DHAVE_ETEXT_SEGMENT=1
EOF
	else
		echo "no"
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
