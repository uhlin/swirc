transfer_site () {
	local dest=maxxe@nifty-networks.net:www-swirc

	printf "  - Checking for rsync..."

	test -x "$(which rsync)" || { printf "error\n"; exit 1; }
	printf "ok\n"

	rsync -rv --progress "${TEMP_DIR}/" "${dest}/"
}
