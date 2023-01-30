#######################################################
#
# Link with Hunspell
#
# Copyright (c) 2023 Markus Uhlin. All rights reserved.
#

link_with_hunspell () {
	local _tmpfile _srcfile _out _libs

	printf "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.cpp"
	_out="${_tmpfile}.out"
	_libs="$(pkg-config --libs-only-l hunspell)"
	cat <<EOF >"$_srcfile"
#include <hunspell/hunspell.hxx>

static Hunspell *hs = nullptr;

int
main(void)
{
	try {
		hs = new Hunspell("", "", nullptr);
	} catch (...) {
		return 1;
	}

	delete hs;
	return 0;
}
EOF

	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	printf "checking for hunspell..."

	${CXX} ${CXXFLAGS} -Werror "$_srcfile" -o "$_out" ${LDFLAGS} ${_libs} \
	    >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DHAVE_HUNSPELL=1
CXXFLAGS += -DHAVE_HUNSPELL=1

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
