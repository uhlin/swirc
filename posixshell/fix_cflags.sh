# fix_cflags.sh

fix_cflags()
{
	if [ -x "$(which pkg-config)" ]; then
		local _hdr_search_path="$(pkg-config --cflags-only-I ncursesw)"

		if [ -n "$_hdr_search_path" ]; then
			echo "CPPFLAGS += ${_hdr_search_path}" >>$MAKE_DEF_FILE
		fi
	fi
}
