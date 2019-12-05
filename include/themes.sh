THEMES="
bx.the
nano.the
superkod.the
themes
"

check_themes () {
	for theme in $THEMES; do
		printf "  - Checking for %s..." "themes/${theme}"
		test -f "themes/${theme}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

add_themes () {
	for theme in $THEMES; do
		printf "  - Adding %s..." "${1}/themes/${theme}"
		cp "themes/${theme}" "${1}/themes/${theme}"

		test -f "${1}/themes/${theme}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
