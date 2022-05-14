PKG = swirc

MSGMERGE = msgmerge
MMFLAGS = --backup=none\
	--sort-output\
	--update

XGETTEXT = xgettext
XGTFLAGS = --add-comments\
	--c++\
	--copyright-holder="Markus Uhlin"\
	--default-domain=$(PKG)\
	--foreign-user\
	--keyword=N_\
	--keyword=_\
	--msgid-bugs-address="https://github.com/uhlin/swirc/issues"\
	--output=$(PKG).pot\
	--sort-output

INPUTFILES = $(SRC_DIR)include/commandhelp.h\
	$(SRC_DIR)io-loop.c\
	$(SRC_DIR)main.cpp\
	$(SRC_DIR)sig-w32.c

POFILES = de/$(PKG).po\
	fi/$(PKG).po\
	fr/$(PKG).po\
	sv/$(PKG).po
MOFILES = de$(SLASH_SYM)$(PKG).mo\
	fi$(SLASH_SYM)$(PKG).mo\
	fr$(SLASH_SYM)$(PKG).mo\
	sv$(SLASH_SYM)$(PKG).mo
