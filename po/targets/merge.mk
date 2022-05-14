merge:
	$(MSGMERGE) $(MMFLAGS) de/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) fi/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) fr/$(PKG).po $(PKG).pot
	$(MSGMERGE) $(MMFLAGS) sv/$(PKG).po $(PKG).pot
