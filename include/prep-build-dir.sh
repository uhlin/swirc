#######################################################
#
# prep-build-dir.sh
#
# Copyright (c) 2022 Markus Uhlin. All rights reserved.
#

check_tools()
{
	local _tools

	_tools="curl"

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

	echo -n "downloading swirc-${UPSTREAM_VER}.tgz..."
	curl --output swirc-${UPSTREAM_VER}.tgz --silent \
	    ${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz
	if [ -f swirc-${UPSTREAM_VER}.tgz ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	echo -n "downloading swirc-${UPSTREAM_VER}.tgz.sig..."
	curl --output swirc-${UPSTREAM_VER}.tgz.sig --silent \
	    ${RELEASES_URL}swirc-${UPSTREAM_VER}.tgz.sig
	if [ -f swirc-${UPSTREAM_VER}.tgz.sig ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	# TODO: verify signature

	echo -n "unpacking swirc-${UPSTREAM_VER}.tgz..."
	tar -xzf swirc-${UPSTREAM_VER}.tgz
	if [ -d swirc-${UPSTREAM_VER} ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	if [ -d swirc-${UPSTREAM_VER}/debian ]; then
		echo "warning: upstream tarball comes with a debian folder"
		echo "deleting it..."
		rm -frv swirc-${UPSTREAM_VER}/debian
	fi

	test -f swirc-${UPSTREAM_VER}/SwircUIBanner1.bmp && \
	    rm -fv swirc-${UPSTREAM_VER}/SwircUIBanner1.bmp

	echo -n "creating swirc_${VERSION}.orig.tar.xz..."
	tar -cJf swirc_${VERSION}.orig.tar.xz swirc-${UPSTREAM_VER}
	if [ -f swirc_${VERSION}.orig.tar.xz ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	echo -n "creating dir: ${BUILD_DIR}..."
	mkdir ${BUILD_DIR}
	if [ -d ${BUILD_DIR} ]; then
		echo "ok"
	else
		echo "error"
		exit 1
	fi

	echo -n "unpacking swirc_${VERSION}.orig.tar.xz..."
	tar -xJ -C ${BUILD_DIR} -f swirc_${VERSION}.orig.tar.xz \
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
