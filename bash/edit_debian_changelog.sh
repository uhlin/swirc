edit_debian_changelog () {
	local author="Markus Uhlin"
	local email="markus.uhlin@bredband.net"
#	local timestamp="$(date -R)"
	local timestamp="$(date "+%a, %e %b %Y %H:%M:%S %z")"
	local tmpFile1="$(mktemp)"
	local tmpFile2="$(mktemp)"

	printf "  - Editing %s..." "$1"

	if [ -f "$1" ]; then
		cat <<EOF > "$tmpFile1"
swirc ($VERSION) unstable; urgency=medium

  * New version

 -- $author <$email>  $timestamp

EOF
		{ cat "$tmpFile1"; cat "$1"; } > "$tmpFile2"
		mv -f "$tmpFile2" "$1" || { echo "error"; exit 1; }
		rm -f "$tmpFile1"      || { echo "error"; exit 1; }
		echo "ok"
	else
		echo "not found"
	fi
}
