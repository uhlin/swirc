CC=cl
CFLAGS=-DNDEBUG=1\
	-DPDC_EXP_EXTRAS=1\
	-DWIN32=1\
	-DWIN32_LEAN_AND_MEAN=1\
	-MD\
	-W3\
	-nologo

# Versions
CURL_VERSION=7.60.0
LIBRESSL_VERSION=2.7.3
PDCURSES_VERSION=3.6

# E and Q
E=@echo
Q=@

MACHINE=x64
NAME_libcrypto=crypto
NAME_libssl=ssl

LDFLAGS=-LIBPATH:curl-$(CURL_VERSION)/$(MACHINE)\
	-LIBPATH:libressl-$(LIBRESSL_VERSION)-windows/$(MACHINE)\
	-LIBPATH:pdcurses-$(PDCURSES_VERSION)/$(MACHINE)\
	-NODEFAULTLIB:MSVCRTD

LDLIBS=$(NAME_libcrypto).lib\
	$(NAME_libssl).lib\
	advapi32.lib\
	libcurl.lib\
	pdcurses.lib\
	user32.lib\
	ws2_32.lib

RM=@del /q
# TEMPFILES=*.obj
