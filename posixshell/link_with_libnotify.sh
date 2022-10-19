link_with_libnotify () {
	local _tmpfile _srcfile _out
	local _includes _libs

	echo -n "creating temp file..."
	_tmpfile=$(mktemp) || { echo "error"; exit 1; }
	echo "ok"

	_srcfile="${_tmpfile}.c"
	_out="${_tmpfile}.out"
	cat <<EOF >"$_srcfile"
#include <libnotify/notify.h>

int
main(void)
{
	NotifyNotification *nn;

	if (!notify_init("Swirc IRC client"))
		return 1;
	(void) nn;
	notify_uninit();
	return 0;
}
EOF
	if [ ! -f "$_srcfile" ]; then
		echo "failed to create $_srcfile"
		exit 1
	fi

	_includes="$(pkg-config --cflags-only-I libnotify)"
	_libs="$(pkg-config --libs-only-l libnotify)"

	echo -n "checking whether to define 'USE_LIBNOTIFY=1'..."
	$CC $CFLAGS $_includes -Werror "$_srcfile" -o "$_out" $LDFLAGS $_libs \
	    >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "yes"
		cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DUSE_LIBNOTIFY=1
CFLAGS += $_includes

CXXFLAGS += -DUSE_LIBNOTIFY=1
CXXFLAGS += $_includes

LDLIBS += $_libs
EOF
	else
		echo "no"
	fi

	echo "cleaning..."
	test -f "$_tmpfile" && rm -f "$_tmpfile"
	test -f "$_srcfile" && rm -f "$_srcfile"
	test -f "$_out" && rm -f "$_out"
}
