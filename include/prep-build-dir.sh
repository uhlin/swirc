#######################################################
#
# prep-build-dir.sh
#
# Copyright (c) 2022 Markus Uhlin. All rights reserved.
#

check_tools()
{
	local _tools

	_tools="curl gpg"

	for _tool in ${_tools}; do
		echo -n "checking for ${_tool}..."
		if [ -x "`/bin/which ${_tool}`" ]; then
			echo "found"
		else
			echo "fatal: not found"
			exit 1
		fi
	done
}

prep_build_dir()
{
	check_tools

	printf "downloading %s..." "swirc_${VERSION}.orig.tar.gz"
	curl --output swirc_${VERSION}.orig.tar.gz --silent \
	    ${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz
	if [ -f swirc_${VERSION}.orig.tar.gz ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	printf "downloading %s..." "swirc_${VERSION}.orig.tar.gz.asc"
	curl --output swirc_${VERSION}.orig.tar.gz.asc --silent \
	    ${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz.sig
	if [ -f swirc_${VERSION}.orig.tar.gz.asc ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	# TODO: verify signature

	echo -n "creating dir: ${BUILD_DIR}..."
	mkdir ${BUILD_DIR}
	if [ -d ${BUILD_DIR} ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	echo -n "unpacking swirc_${VERSION}.orig.tar.xz..."
	tar -xz -C ${BUILD_DIR} -f swirc_${VERSION}.orig.tar.gz \
	    --strip-components=1
	if [ -f ${BUILD_DIR}/configure ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	echo -n "copying files to ${BUILD_DIR}/debian..."
	cp -R debian ${BUILD_DIR}
	if [ -d ${BUILD_DIR}/debian ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi
}
