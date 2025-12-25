CC = cl
CFLAGS = -MT -O2 -W3 -experimental:c11atomics -nologo -std:c17

CXX = $(CC)
CXXFLAGS = -DUNICODE=1 -EHa -MT -O2 -W3 -Zc:__cplusplus -nologo -std:c++17

# C preprocessor flags
CPPFLAGS = -DHAVE_ATLSTR_H=1\
	-DHAVE_HUNSPELL=1\
	-DHAVE_LIBICONV=1\
	-DHAVE_LIBIDN=1\
	-DHAVE_LIBINTL_H=1\
	-DHAVE_LIBINTL_SETLOCALE=1\
	-DHAVE_STRCASESTR=0\
	-DHAVE_TIMINGSAFE_MEMCMP=0\
	-DNDEBUG=1\
	-DPDC_EXP_EXTRAS=1\
	-DPDC_NCMOUSE=1\
	-DTOAST_NOTIFICATIONS=1\
	-DWIN32=1\
	-DWIN32_LEAN_AND_MEAN=1

# Versions
CURL_VERSION = 8.17.0
HUNSPELL_VERSION = 1.7.2
LIBRESSL_VERSION = 4.2.1
PDCURSES_VERSION = 3.9

GNU_BUNDLE_DATE = 202205
LOCALES_SNAP = 20251214

# E and Q
E = @echo
Q = @

MACHINE = x64
NAME_libcharset = libcharset-1
NAME_libcrypto = crypto
NAME_libhunspell = libhunspell-1.7-0
NAME_libiconv = libiconv-2
NAME_libidn = libidn-12
NAME_libintl = libintl-8
NAME_libssl = ssl

# LibreSSL DLLs
DLL_crypto = crypto-57.dll
DLL_ssl = ssl-60.dll

# MINGW hunspell
DLL_libgcc = libgcc_s_seh-1.dll
DLL_libstdcpp = libstdc++-6.dll
DLL_libwinpthread = libwinpthread-1.dll

LDFLAGS = -LIBPATH:curl-$(CURL_VERSION)/$(MACHINE)\
	-LIBPATH:gnu-bundle-$(GNU_BUNDLE_DATE)/$(MACHINE)\
	-LIBPATH:hunspell-$(HUNSPELL_VERSION)/$(MACHINE)\
	-LIBPATH:libressl-$(LIBRESSL_VERSION)/$(MACHINE)\
	-LIBPATH:pdcurses-$(PDCURSES_VERSION)/$(MACHINE)

LDLIBS = $(NAME_libcharset).lib\
	$(NAME_libcrypto).lib\
	$(NAME_libhunspell).lib\
	$(NAME_libiconv).lib\
	$(NAME_libidn).lib\
	$(NAME_libintl).lib\
	$(NAME_libssl).lib\
	advapi32.lib\
	bcrypt.lib\
	kernel32.lib\
	libcurl.lib\
	pdcurses.lib\
	runtimeobject.lib\
	user32.lib\
	ws2_32.lib

RM = @del /q
SLASH_SYM = ^\
