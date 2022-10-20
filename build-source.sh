#!/bin/bash

source "vars.sh"

source "include/clean-up.sh"
source "include/prep-build-dir.sh"

clean_up
prep_build_dir

cd ${BUILD_DIR}
dpkg-source --build .
cd ..
