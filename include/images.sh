IMAGES="
blue-globe.ico
blue-globe.png
debian128X128.png
download82-badge1.png
download82_clean_award_128617.png
openbsd128X128.png
softpedia_download_large_shadow.png
sp100clean.png
swirc-royal-110x110.png
swirc-royal.ico
swirc-royal.png
void128X128.png
windows128X128.png
"

check_images () {
	for img in $IMAGES; do
		printf "  - Checking for %s..." "gfx/${img}"
		test -f "gfx/${img}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}

add_images () {
	for img in $IMAGES; do
		printf "  - Adding %s..." "${1}/gfx/${img}"
		cp "gfx/${img}" "${1}/gfx/${img}"

		test -f "${1}/gfx/${img}" || { printf "error\n"; exit 1; }
		printf "ok\n"
	done
}
