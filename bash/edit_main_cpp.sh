edit_main_cpp () {
	local _expr="s/g_swircVersion\[\] = \"v.*\"/g_swircVersion\[\] = \"v$VERSION\"/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
