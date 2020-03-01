edit_rc_file () {
	local regex1="#define VER [0-9]\,[0-9]\,[0-9]\,[0-9]"
	local regex2="#define VER_STR \".*\""
	local sub1="#define VER $MAJOR_VERSION\,$MINOR_VERSION\,$PATCHLEVEL\,0"
	local sub2="#define VER_STR \"${VERSION}\\\\0\""

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "s/$regex1/$sub1/" "$1" || { echo "error"; exit 1; }
		sed -i "s/$regex2/$sub2/" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
