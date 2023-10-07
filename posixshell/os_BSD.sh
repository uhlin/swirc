# --------------------------------------------------
#    Generic BSD (Berkeley Software Distribution)
# --------------------------------------------------

os_BSD () {
	cat <<EOF >>"${MAKE_DEF_FILE}"
CC = cc
CFLAGS = -O2 -Wall -pipe -std=c17

CXX = c++
CXXFLAGS = -O2 -Wall -pipe -std=c++17

CPPFLAGS = -DBSD=1\\
	-DNDEBUG=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include

LDFLAGS = -L/usr/local/lib
LDLIBS = -lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF

	if [ -z ${CC+x} ]; then
		CC=cc
	fi
	if [ -z ${CXX+x} ]; then
		CXX=c++
	fi
	set_common
}
