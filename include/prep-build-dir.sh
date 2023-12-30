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
		printf "checking for %s..." "${_tool}"
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

	printf "creating dir: %s..." "${BUILD_DIR}"
	mkdir ${BUILD_DIR}
	if [ -d ${BUILD_DIR} ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	printf "unpacking %s..." "swirc_${VERSION}.orig.tar.gz"
	tar -xz -C ${BUILD_DIR} -f swirc_${VERSION}.orig.tar.gz \
	    --strip-components=1
	if [ -f ${BUILD_DIR}/configure ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	printf "copying files to %s..." "${BUILD_DIR}/debian"
	cp -R debian ${BUILD_DIR}
	if [ -d ${BUILD_DIR}/debian ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi
}
