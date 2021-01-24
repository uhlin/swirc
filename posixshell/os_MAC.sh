# --------------------------------------------------
# 		     Mac OS X
# --------------------------------------------------

os_MAC () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DNCURSES_OPAQUE=0\\
	-DOS_X=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/opt/libressl/include\\
	-O2\\
	-Wall\\
	-pipe
CC=clang
CFLAGS=\$(SHARED_FLAGS) -std=c17
CXX=clang++
CXXFLAGS=\$(SHARED_FLAGS) -std=c++17
LDFLAGS=-L/usr/local/opt/libressl/lib
LDLIBS=-lcrypto\\
	-lcurl\\
	-lncurses\\
	-lpanel\\
	-lpthread\\
	-lssl
EOF
}
