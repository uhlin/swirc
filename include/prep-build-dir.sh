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
	local _out1="swirc_${VERSION}.orig.tar.gz"
	local _out2="swirc_${VERSION}.orig.tar.gz.asc"
	local _url1="${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz"
	local _url2="${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz.sig"

	check_tools

	printf "downloading %s..." "${_out1}"
	curl --output ${_out1} --silent ${_url1}
	if [ -f ${_out1} ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	printf "downloading %s..." "${_out2}"
	curl --output ${_out2} --silent ${_url2}
	if [ -f ${_out2} ]; then
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

	printf "unpacking %s..." "${_out1}"
	if [[ ${_out1} = *.tar.gz ]]; then
		tar -xz -C ${BUILD_DIR} -f ${_out1} --strip-components=1
	elif [[ ${_out1} = *.tar.xz ]]; then
		tar -xJ -C ${BUILD_DIR} -f ${_out1} --strip-components=1
	else
		echo "error"
		exit 1
	fi

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
