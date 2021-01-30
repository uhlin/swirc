# --------------------------------------------------
# 		      NetBSD
# --------------------------------------------------

os_NETBSD () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DBSD=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/pkg/include/ncursesw\\
	-I/usr/pkg/include\\
	-Wall
CC=cc
CFLAGS=\$(SHARED_FLAGS) -std=c11
CXX=g++
CXXFLAGS=\$(SHARED_FLAGS) -std=c++17
LDFLAGS=-L/usr/pkg/lib\\
	-Wl,-rpath,/usr/pkg/lib
LDLIBS=-lcrypto\\
	-lcurl\\
	-lgnupanelw\\
	-lncursesw\\
	-lpthread\\
	-lssl
EOF
}
