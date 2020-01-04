PAGES="
about.html
cmds.html
debian.html
doc.html
index.html
openbsd.html
source-code.html
windows.html
"

check_pages () {
	for pg in $PAGES; do
		printf "  - Checking for %s..." "templates/${pg}"
		test -r "templates/${pg}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

gen_pages () {
	for pg in $PAGES; do
		printf "  - Generating %s..." "${1}/${pg}"

		sed -e "/-----HEAD COMMON TAGS-----/r head_common_tags.html"\
		    -e "/-----HEAD COMMON TAGS-----/d"\
		    -e "/-----TOP-----/r top.html"\
		    -e "/-----TOP-----/d"\
		    -e "/-----MENU-----/r menu.html"\
		    -e "/-----MENU-----/d"\
		    -e "/-----BOTTOM-----/r bottom.html"\
		    -e "/-----BOTTOM-----/d"\
		    "templates/${pg}" > "${1}/${pg}"

		test -f "${1}/${pg}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
