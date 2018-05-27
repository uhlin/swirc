INSTALL=install
INSTALL_DEPS=swirc src/swirc.1
PREFIX?=/usr/local

# Don't provide a default value for DESTDIR. It should be empty.
DESTDIR?=

DEST_PROGRAM=$(DESTDIR)$(PREFIX)/bin
DEST_MANUAL=$(DESTDIR)$(PREFIX)/man/man1

install: $(INSTALL_DEPS)
	$(INSTALL) -d $(DEST_PROGRAM)
	$(INSTALL) -d $(DEST_MANUAL)
	$(INSTALL) -m 0755 swirc $(DEST_PROGRAM)
	$(INSTALL) -m 0444 src/swirc.1 $(DEST_MANUAL)
