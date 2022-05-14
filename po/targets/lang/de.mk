################
##   German   ##
################

de/$(PKG).po:
	mkdir -p de
	msginit --input=$(PKG).pot --output-file=de/$(PKG).po --locale=de_DE.UTF-8
de/$(PKG).mo:
	msgfmt --output-file=de/$(PKG).mo de/$(PKG).po
