# --------------------------------------------------
# 		    GNU/Linux
# --------------------------------------------------

os_LINUX () {
	cat <<EOF >>"${MAKE_DEF_FILE}"
CC = gcc
CFLAGS = -O2\\
	-Wall\\
	-Wformat-security\\
	-Wshadow\\
	-Wsign-compare\\
	-Wstrict-prototypes\\
	-fstack-protector-strong\\
	-g\\
	-pipe\\
	-std=c11

CXX = g++
CXXFLAGS = -O2 -Wall -fstack-protector-strong -g -pipe -std=c++17

# C preprocessor flags
CPPFLAGS = -DLINUX=1\\
	-DNDEBUG=1\\
	-DUNIX=1\\
	-D_FORTIFY_SOURCE=3\\
	-D_POSIX_C_SOURCE=200809L\\
	-D_XOPEN_SOURCE=500\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include

LDFLAGS = -L/usr/local/lib\\
	-Wl,-z,now\\
	-Wl,-z,relro
LDLIBS = -lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF

	if [ -z ${CC+x} ]; then
		CC=gcc
	fi
	if [ -z ${CXX+x} ]; then
		CXX=g++
	fi
	set_common
}
