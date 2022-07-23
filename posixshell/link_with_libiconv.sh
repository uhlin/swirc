link_with_libiconv () {
	local _tmpfile _srcfile _out _libs

	echo -n "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <iconv.h>
#include <stdio.h>

int
main(void)
{
	iconv_t cd;

	if ((cd = iconv_open("ISO-8859-1", "UTF-8")) == ((iconv_t) -1))
		return 1;
	else if (iconv_close(cd) != 0)
		return 1;
	puts("iconv works!");
	return 0;
}
EOF
	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi
	echo -n "checking for gnu libiconv..."
	if [ "$(uname -s)" = "Linux" ]; then
		_libs=""
	else
		_libs="-liconv"
	fi
	$CC $CFLAGS "$_srcfile" -o "$_out" $LDFLAGS $_libs >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DHAVE_LIBICONV=1
CXXFLAGS += -DHAVE_LIBICONV=1
EOF
	else
		echo "no"
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
