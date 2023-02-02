set_common () {
	if [ -z ${CFLAGS+x} ]; then
		CFLAGS="-I/usr/local/include"
	fi
	if [ -z ${CXXFLAGS+x} ]; then
		CXXFLAGS="-I/usr/local/include"
	fi
	if [ -z ${LDFLAGS+x} ]; then
		LDFLAGS="-L/usr/local/lib"
	fi
}
