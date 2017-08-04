E=@echo
Q=@
RM=@del /q
TEMPFILES=*.obj
CC=cl
CFLAGS=-DNDEBUG=1 -DPDC_EXP_EXTRAS=1 -DWIN32=1 -DWIN32_LEAN_AND_MEAN=1 -MT -W3 -nologo
LDFLAGS=
NAME_libcrypto=libcrypto-41
NAME_libssl=libssl-43
LDLIBS=$(NAME_libcrypto).lib $(NAME_libssl).lib libcurl.lib pdcurses.lib ws2_32.lib
CURL_VERSION=7.54.1
LIBRESSL_VERSION=2.5.4
