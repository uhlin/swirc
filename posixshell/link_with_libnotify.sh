link_with_libnotify () {
	local _s1="$(pkg-config --cflags-only-I libnotify)"
	local _s2="$(pkg-config --libs-only-l libnotify)"

	cat <<EOF >>$MAKE_DEF_FILE
CFLAGS+=-DUSE_LIBNOTIFY=1
CFLAGS+=$_s1

CXXFLAGS+=-DUSE_LIBNOTIFY=1
CXXFLAGS+=$_s1

LDLIBS+=$_s2
EOF
}
