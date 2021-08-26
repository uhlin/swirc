edit_po_file () {
	local regex="Project-Id-Version: swirc [0-9]\.[0-9]\.[0-9]"
	local sub="Project-Id-Version: swirc $MAJOR_VERSION\.$MINOR_VERSION\.$PATCHLEVEL"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "s/$regex/$sub/" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
