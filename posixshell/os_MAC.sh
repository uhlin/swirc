# --------------------------------------------------
# 		     Mac OS X
# --------------------------------------------------

os_MAC () {
	cat <<EOF >>"${MAKE_DEF_FILE}"
CC = clang
CFLAGS = -O2 -Wall -pipe -std=c11

CXX = clang++
CXXFLAGS = -O2 -Wall -pipe -std=c++17

# C preprocessor flags
CPPFLAGS = -DNCURSES_OPAQUE=0\\
	-DNDEBUG=1\\
	-DOS_X=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include\\
	-I/usr/local/opt/libressl/include

LDFLAGS = -L/usr/local/lib\\
	-L/usr/local/opt/libressl/lib
LDLIBS = -lcrypto\\
	-lcurl\\
	-lncurses\\
	-lpanel\\
	-lpthread\\
	-lssl
EOF

	if [ -z ${CC+x} ]; then
		CC=clang
	fi
	if [ -z ${CXX+x} ]; then
		CXX=clang++
	fi
	set_common
}
