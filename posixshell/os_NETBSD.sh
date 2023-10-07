# --------------------------------------------------
# 		      NetBSD
# --------------------------------------------------

os_NETBSD () {
	cat <<EOF >>"${MAKE_DEF_FILE}"
CC = cc
CFLAGS = -O2 -Wall -std=c11

CXX = c++
CXXFLAGS = -O2 -Wall -std=c++17

# C preprocessor flags
CPPFLAGS = -DBSD=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include\\
	-I/usr/pkg/include/ncursesw\\
	-I/usr/pkg/include

LDFLAGS = -L/usr/local/lib\\
	-L/usr/pkg/lib\\
	-Wl,-rpath,/usr/pkg/lib
LDLIBS = -lcrypto\\
	-lcurl\\
	-lgnupanelw\\
	-lncursesw\\
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
