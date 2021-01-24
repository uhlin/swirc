# --------------------------------------------------
# 	 GNU/Linux (with Sun C compiler)
# --------------------------------------------------

os_LINUX_suncc () {
	cat <<EOF >>$MAKE_DEF_FILE
SHARED_FLAGS=-DLINUX=1\\
	-DUNIX=1\\
	-D_FORTIFY_SOURCE=2\\
	-D_POSIX_C_SOURCE=200809L\\
	-D_XOPEN_SOURCE=500\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-O2\\
	-Wp,-I/usr/include/x86_64-linux-gnu\\
	-errtags\\
	-pedantic\\
	-xannotate\\
	-xatomic=studio\\
	-xprevise\\
	-xsecure_code_analysis
CC=suncc
CFLAGS=\$(SHARED_FLAGS) -std=c11
CXX=sunCC
CXXFLAGS=\$(SHARED_FLAGS) -std=c++14
LDFLAGS=-xannotate -xprevise
LDLIBS=-lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF
}
