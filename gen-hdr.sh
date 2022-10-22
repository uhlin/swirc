#!/bin/sh

if [ $# -ne 1 ]; then
	echo "bogus number of args"
	exit 1
fi

HDRPATH=src/include/swircpaths.h

PREFIX=$1
shift

cat <<EOF >${HDRPATH}
#ifndef _SWIRCPATHS_H_
#define _SWIRCPATHS_H_

#define SWIRC_BTD_PATH "${PREFIX}/share/locale"
#define SWIRC_ICON_PATH "${PREFIX}/share/swirc/swirc-royal.png"

#define LC_MSGS_DE	"${PREFIX}/share/locale/de/LC_MESSAGES/swirc.mo"
#define LC_MSGS_FI	"${PREFIX}/share/locale/fi/LC_MESSAGES/swirc.mo"
#define LC_MSGS_FR	"${PREFIX}/share/locale/fr/LC_MESSAGES/swirc.mo"
#define LC_MSGS_SV	"${PREFIX}/share/locale/sv/LC_MESSAGES/swirc.mo"

#endif
EOF

if [ ! -r ${HDRPATH} ]; then
	echo "fatal: error creating ${HDRPATH}"
	exit 1
fi
