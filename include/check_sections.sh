SECTIONS="
bottom.html
head_common_tags.html
menu.html
top.html
"

check_sections () {
	for s in $SECTIONS; do
		printf "  - Checking for %s..." "$s"
		test -r "$s" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
