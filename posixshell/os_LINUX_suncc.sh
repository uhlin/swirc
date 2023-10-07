# --------------------------------------------------
# 	 GNU/Linux (with Sun C compiler)
# --------------------------------------------------

os_LINUX_suncc () {
	cat <<EOF >>${MAKE_DEF_FILE}
CC = suncc
CFLAGS = -O2\\
	-Wp,-I/usr/include/x86_64-linux-gnu\\
	-errtags\\
	-pedantic\\
	-std=c11\\
	-xannotate\\
	-xatomic=studio\\
	-xprevise\\
	-xsecure_code_analysis

CXX = sunCC
CXXFLAGS = -O2\\
	-Wp,-I/usr/include/x86_64-linux-gnu\\
	-errtags\\
	-pedantic\\
	-std=c++14\\
	-xannotate\\
	-xatomic=studio\\
	-xprevise\\
	-xsecure_code_analysis

CPPFLAGS = -DLINUX=1\\
	-DUNIX=1\\
	-D_FORTIFY_SOURCE=2\\
	-D_POSIX_C_SOURCE=200809L\\
	-D_XOPEN_SOURCE=500\\
	-D_XOPEN_SOURCE_EXTENDED=1\\
	-I/usr/local/include

LDFLAGS = -L/usr/local/lib\\
	-xannotate\\
	-xprevise
LDLIBS = -lcrypto\\
	-lcurl\\
	-lncursesw\\
	-lpanelw\\
	-lpthread\\
	-lssl
EOF

	if [ -z ${CC+x} ]; then
		CC=suncc
	fi
	if [ -z ${CXX+x} ]; then
		CXX=sunCC
	fi
	set_common
}
