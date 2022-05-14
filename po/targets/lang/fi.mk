#################
##   Finnish   ##
#################

fi/$(PKG).po:
	mkdir -p fi
	msginit --input=$(PKG).pot --output-file=fi/$(PKG).po --locale=fi_FI.UTF-8
fi/$(PKG).mo:
	msgfmt --output-file=fi/$(PKG).mo fi/$(PKG).po
