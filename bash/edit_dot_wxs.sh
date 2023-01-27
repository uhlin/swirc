edit_dot_wxs () {
	local _expr1="16s/    Name=\".*\"/    Name=\"Swirc $VERSION\"/"
	local _expr2="18s/    Version=\".*\"/    Version=\"${VERSION}\.0\"/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr1" "$1" || { echo "error"; exit 1; }
		sed -i "$_expr2" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
