# --------------------------------------------------
#    Generic BSD (Berkeley Software Distribution)
# --------------------------------------------------

os_BSD () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DBSD=1\\
	-DNDEBUG=1\\
	-DUNIX=1\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include\\
	-O2\\
	-Wall\\
	-pipe
CC=cc
CFLAGS=\$(SHARED_FLAGS) -std=c17
CXX=c++
CXXFLAGS=\$(SHARED_FLAGS) -std=c++17
LDFLAGS=-L/usr/local/lib
LDLIBS=-lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF

	if [ -z ${CC+x} ]; then
		CC=cc
	fi
	if [ -z ${CFLAGS+x} ]; then
		CFLAGS="-I/usr/local/include"
	fi
}
