edit_main_cpp () {
	local t="$(echo x | tr 'x' '\t')"
	local _expr="s/g_swircVersion$t= \"v.*\"/g_swircVersion$t= \"v$VERSION\"/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
