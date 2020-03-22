OTHER_FILES="
mu170225.asc
mu180520.asc
mu190523.asc
navbar.css
reports/swirc-3.2.2.html
reports/swirc-3.2.3.html
style.css
swirc.list
"

check_other_files () {
	for file in $OTHER_FILES; do
		printf "  - Checking for %s..." "$file"
		test -f "$file" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

add_other_files () {
	for file in $OTHER_FILES; do
		printf "  - Adding %s..." "${1}/${file}"
		cp "$file" "${1}/${file}"

		test -f "${1}/${file}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
