THEMES="
bx
nano
superkod
weechat
"

check_themes () {
	local _suff=".thm"

	for theme in $THEMES; do
		printf "  - Checking for %s..." "themes/${theme}${_suff}"
		test -f "themes/${theme}${_suff}" || \
		    { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

add_themes () {
	local _suff="${2}"

	for theme in $THEMES; do
		printf "  - Adding %s..." "${1}/themes/${theme}${_suff}"
		cp "themes/${theme}.thm" "${1}/themes/${theme}${_suff}"

		test -f "${1}/themes/${theme}${_suff}" || \
		    { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
