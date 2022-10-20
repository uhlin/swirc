clean_up()
{
	local _dirs _files

	_dirs="
${BUILD_DIR}
swirc-${UPSTREAM_VER}
"
	_files="
swirc-${UPSTREAM_VER}.tgz
swirc-${UPSTREAM_VER}.tgz.sig
swirc_${DEB_VERSION}.debian.tar.xz
swirc_${DEB_VERSION}.dsc
swirc_${VERSION}.orig.tar.xz
"

	echo "deleting directories..."
	for _d in ${_dirs}; do
		if [ -d ${_d} ]; then
			rm -rf ${_d}
			echo "deleted ${_d}"
		fi
	done

	echo "deleting files..."
	for _f in ${_files}; do
		test -f ${_f} && rm -fv ${_f}
	done

	rm -fv swirc*.buildinfo
	rm -fv swirc*.changes
	rm -fv swirc*.deb
}
