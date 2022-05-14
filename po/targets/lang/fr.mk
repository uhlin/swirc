################
##   French   ##
################

fr/$(PKG).po:
	mkdir -p fr
	msginit --input=$(PKG).pot --output-file=fr/$(PKG).po --locale=fr_FR.UTF-8
fr/$(PKG).mo:
	msgfmt --output-file=fr/$(PKG).mo fr/$(PKG).po
