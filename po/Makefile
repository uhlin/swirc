ROOT = ../
COMMANDS_DIR := $(ROOT)src/commands/
EVENTS_DIR := $(ROOT)src/events/
SRC_DIR := $(ROOT)src/

PKG := swirc

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
	--package-name=$(PKG)\
	--package-version=3.3.1\
	--sort-output

INPUTFILES = $(SRC_DIR)main.cpp

POFILES = de/$(PKG).po\
	fi/$(PKG).po\
	fr/$(PKG).po\
	sv/$(PKG).po
MOFILES = de/$(PKG).mo\
	fi/$(PKG).mo\
	fr/$(PKG).mo\
	sv/$(PKG).mo

include $(ROOT)options.mk

all: main

merge:
	$(MSGMERGE) $(MMFLAGS) de/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) fi/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) fr/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) sv/$(PKG).po $(PKG).pot

main: $(PKG).pot $(POFILES) $(MOFILES)

$(PKG).pot:
	$(Q) $(XGETTEXT) $(XGTFLAGS) $(INPUTFILES)

################
##   German   ##
################

de/$(PKG).po:
	mkdir -p de
	msginit --input=$(PKG).pot --output-file=de/$(PKG).po --locale=de_DE.UTF-8
de/$(PKG).mo:
	msgfmt --output-file=de/$(PKG).mo de/$(PKG).po

#################
##   Finnish   ##
#################

fi/$(PKG).po:
	mkdir -p fi
	msginit --input=$(PKG).pot --output-file=fi/$(PKG).po --locale=fi_FI.UTF-8
fi/$(PKG).mo:
	msgfmt --output-file=fi/$(PKG).mo fi/$(PKG).po

################
##   French   ##
################

fr/$(PKG).po:
	mkdir -p fr
	msginit --input=$(PKG).pot --output-file=fr/$(PKG).po --locale=fr_FR.UTF-8
fr/$(PKG).mo:
	msgfmt --output-file=fr/$(PKG).mo fr/$(PKG).po

#################
##   Swedish   ##
#################

sv/$(PKG).po:
	mkdir -p sv
	msginit --input=$(PKG).pot --output-file=sv/$(PKG).po --locale=sv_SE.UTF-8
sv/$(PKG).mo:
	msgfmt --output-file=sv/$(PKG).mo sv/$(PKG).po

clean:
	$(E) "  CLEAN"
	$(RM) $(PKG).pot
	$(RM) $(MOFILES)
