edit_vc_makefile () {
	local _expr1="s/PRODUCT_VERSION = .*/PRODUCT_VERSION = $VERSION/"
	local _expr2="s/REVISION =.*/REVISION =/"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "$_expr1" "$1" || { echo "error"; exit 1; }
		sed -i "$_expr2" "$1" || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
