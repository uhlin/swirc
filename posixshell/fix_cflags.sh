fix_cflags () {
	local _flags _helper_scripts

	_helper_scripts="
ncursesw6-config
ncursesw5-config
"

	for s in $_helper_scripts; do
		if [ -x "$(which "$s")" ]; then
			_flags="$($s --cflags)"

			if [ -n "$_flags" ]; then
				echo "CFLAGS += ${_flags}" >>$MAKE_DEF_FILE
				echo "CXXFLAGS += ${_flags}" >>$MAKE_DEF_FILE
			fi

			break
		fi
	done
}

fix_cflags_with_pkg_config () {
	local _hdr_search_path

	if [ -x "$(which pkg-config)" ]; then
		_hdr_search_path="$(pkg-config --cflags-only-I ncursesw)"

		if [ -n "$_hdr_search_path" ]; then
			echo "CFLAGS += ${_hdr_search_path}" >>$MAKE_DEF_FILE
			echo "CXXFLAGS += ${_hdr_search_path}" >>$MAKE_DEF_FILE
		fi
	fi
}
