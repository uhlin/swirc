# --------------------------------------------------
# 		    GNU/Linux
# --------------------------------------------------

os_LINUX () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DLINUX=1\\
	-DUNIX=1\\
	-D_FORTIFY_SOURCE=2\\
	-D_POSIX_C_SOURCE=200809L\\
	-D_XOPEN_SOURCE=500\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-O2\\
	-Wall\\
	-fstack-protector-strong\\
	-g\\
	-pipe
CC=gcc
CFLAGS=\$(SHARED_FLAGS) -std=c17\\
	-Wsign-compare -Wstrict-prototypes
CXX=g++
CXXFLAGS=\$(SHARED_FLAGS) -std=c++17
LDFLAGS=-Wl,-z,now\\
	-Wl,-z,relro
LDLIBS=-lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF
}
