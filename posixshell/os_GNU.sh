# --------------------------------------------------
# 		     GNU Hurd
# --------------------------------------------------

os_GNU () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DHURD=1\\
	-DNDEBUG=1\\
	-DUNIX=1\\
	-D_FORTIFY_SOURCE=2\\
	-D_GNU_SOURCE=1\\
	-O2\\
	-Wall\\
	-fstack-protector-strong\\
	-g\\
	-pipe
CC=gcc
CFLAGS=\$(SHARED_FLAGS) -std=c11\\
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

	if [ -z ${CC+x} ]; then
		CC=gcc
	fi
	set_common
}
