DIRS="
gfx
reports
screenshots
themes
"

check_dirs () {
	for dir in $DIRS; do
		printf "  - Checking for %s..." "$dir"
		test -d "$dir" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

create_dirs () {
	for dir in $DIRS; do
		printf "  - Creating %s..." "${1}/${dir}"

		if test -d "${1}/${dir}"; then
			printf "already existent\n"
		else
			mkdir -p "${1}/${dir}" || { printf "error\n"; exit 1; }
			printf "ok\n"
		fi
	done
}
