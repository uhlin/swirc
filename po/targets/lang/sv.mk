#################
##   Swedish   ##
#################

sv/$(PKG).po:
	mkdir -p sv
	msginit --input=$(PKG).pot --output-file=sv/$(PKG).po --locale=sv_SE.UTF-8
sv/$(PKG).mo:
	msgfmt --output-file=sv/$(PKG).mo sv/$(PKG).po
