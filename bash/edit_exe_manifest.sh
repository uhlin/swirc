edit_exe_manifest () {
	local _expr="19s/version=\".*\"/version=\"${VERSION}\.0\"/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
