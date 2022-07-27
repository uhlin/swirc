edit_ports_makefile () {
	local _expr="2s/swirc-.*/swirc-$VERSION/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
