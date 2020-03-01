edit_changelog () {
	local _date="$(date +%Y-%m-%d)"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		sed -i "s/## \[Unreleased\] ##/## \[$VERSION\] - $_date ##/" "$1" || \
		    { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
