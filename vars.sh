BUILD_DIR=build
DEB_VERSION=`dpkg-parsechangelog --show-field Version`
RELEASES_URL="https://www.nifty-networks.net/swirc/releases/"
UPSTREAM_VER=3.3.6
VERSION=`dpkg-parsechangelog --show-field Version | cut -f1 -d'-'`
