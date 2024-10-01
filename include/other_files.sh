OTHER_FILES="
dic_db
mu190523.asc
mu231230.pgp
navbar.css
popup.css
reports/swirc-3.3.0.html
reports/swirc-3.3.2.html
style.css
swirc.list
themes/themes
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