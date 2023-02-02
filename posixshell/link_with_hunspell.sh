#######################################################
#
# Link with Hunspell
#
# Copyright (c) 2023 Markus Uhlin. All rights reserved.
#

link_with_hunspell () {
	local _tmpfile _srcfile _out
	local _includes _libpath _libs

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.cpp"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <cstddef>
#include <hunspell/hunspell.h>

int
main(void)
{
	Hunhandle *hh;

	if ((hh = Hunspell_create("", "")) != NULL)
		Hunspell_destroy(hh);
	return 0;
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	_includes="$(pkg-config --cflags-only-I hunspell)"
	_libpath="$(pkg-config --libs-only-L hunspell)"
	_libs="$(pkg-config --libs-only-l hunspell)"

	printf "checking for hunspell..."

	${CXX} ${CXXFLAGS} ${_includes%%/hunspell} -Werror "$_srcfile" -o \
	    "$_out" ${LDFLAGS} ${_libpath} ${_libs} >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DHAVE_HUNSPELL=1
CFLAGS += ${_includes%%/hunspell}

CXXFLAGS += -DHAVE_HUNSPELL=1
CXXFLAGS += ${_includes%%/hunspell}

LDFLAGS += ${_libpath}
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
