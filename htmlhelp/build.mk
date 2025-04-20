HTMLHELP_DIR = htmlhelp^\
TGTS = $(TGTS) $(HTMLHELP_DIR)swirc.chm

CHM_DEPS = $(HTMLHELP_DIR)about.htm\
	$(HTMLHELP_DIR)confopts.htm\
	$(HTMLHELP_DIR)progopts.htm\
	$(HTMLHELP_DIR)style.css\
	$(HTMLHELP_DIR)swirc.hhc\
	$(HTMLHELP_DIR)swirc.hhk\
	$(HTMLHELP_DIR)swirc.hhp

$(HTMLHELP_DIR)swirc.chm: $(CHM_DEPS)
	hhc $(HTMLHELP_DIR)swirc.hhp
