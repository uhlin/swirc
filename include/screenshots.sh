SCREENSHOTS="
swirc-310-1.png
swirc-310-2.png
swirc-310-3.png
swirc-311-1.png
swirc-311-2.png
swirc-311-3.png
"

check_screenshots () {
	for img in $SCREENSHOTS; do
		printf "  - Checking for %s..." "screenshots/${img}"
		test -f "screenshots/${img}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

add_screenshots () {
	for img in $SCREENSHOTS; do
		printf "  - Adding %s..." "${1}/screenshots/${img}"
		cp "screenshots/${img}" "${1}/screenshots/${img}"

		test -f "${1}/screenshots/${img}" || \
		    { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
