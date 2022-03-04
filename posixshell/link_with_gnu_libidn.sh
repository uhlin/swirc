link_with_gnu_libidn () {
	cat <<EOF >>$MAKE_DEF_FILE
CFLAGS += -DHAVE_LIBIDN=1
CXXFLAGS += -DHAVE_LIBIDN=1

LDLIBS += -lidn
EOF
}
