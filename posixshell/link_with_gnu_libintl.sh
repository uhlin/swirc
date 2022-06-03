check_intl_header () {
	local _tmpfile _srcfile _out

	echo -n "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <libintl.h>

int
main(void)
{
	return 0;
}
EOF
	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi
	echo -n "checking for 'libintl.h'..."
	$CC $CFLAGS "$_srcfile" -o "$_out" >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DHAVE_LIBINTL_H=1
CXXFLAGS += -DHAVE_LIBINTL_H=1
EOF
		case "$(uname -s)" in
		"Darwin" | "FreeBSD" | "NetBSD" | "OpenBSD")
			cat <<EOF >>$MAKE_DEF_FILE
LDLIBS += -lintl
EOF
			;;
		*)
			;;
		esac
	else
		echo "no"
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}

check_intl_setlocale () {
	true
}

link_with_gnu_libintl () {
	check_intl_header
	check_intl_setlocale
}
