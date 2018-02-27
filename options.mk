CC=gcc
CFLAGS=\
	-DBSD=1\
	-DUNIX=1\
	-Wall\
	-std=c99\
	-D_XOPEN_SOURCE_EXTENDED=1\
	-I/usr/pkg/include\
	-I/usr/pkg/include/ncurses\
	-I/usr/pkg/include/ncursesw

LDFLAGS=-L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib\
	-lcrypto\
	-lcurl\
	-lncursesw\
	-lgnupanelw\
	-lpthread\
	-lssl
