#!/bin/bash

if [[ $# -lt 1 ]]; then
	echo "./build-package.sh source|any|all|binary|full [args]"
	exit 1
fi

BUILD=$1
shift

source "vars.sh"

source "include/clean-up.sh"
source "include/prep-build-dir.sh"

clean_up
prep_build_dir

cd ${BUILD_DIR} || exit 1
fakeroot -- dpkg-buildpackage --build=$BUILD --no-sign "$@"
cd ..
