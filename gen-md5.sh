#!/bin/bash

DESTDIR=debian/tmp
PREFIX=./usr

gen_md5()
{
	local _md5 _files

	_md5="DEBIAN/md5sums"
	_files="
${PREFIX}/bin/swirc
${PREFIX}/share/doc/swirc/changelog.Debian.gz
${PREFIX}/share/doc/swirc/copyright
${PREFIX}/share/locale/de/LC_MESSAGES/swirc.mo
${PREFIX}/share/locale/fi/LC_MESSAGES/swirc.mo
${PREFIX}/share/locale/fr/LC_MESSAGES/swirc.mo
${PREFIX}/share/locale/sv/LC_MESSAGES/swirc.mo
${PREFIX}/share/man/man1/swirc.1.gz
${PREFIX}/share/man/man5/swirc.conf.5.gz
${PREFIX}/share/swirc/swirc-royal.png
"

	if [ ! -d "DEBIAN" ]; then
		echo "cannot find the 'DEBIAN' folder"
		exit 1
	fi

	cat /dev/null >${_md5}

	for _f in ${_files}; do
		test -f ${_f} && md5sum ${_f} >>${_md5}
	done
}

if [ ! -d ${DESTDIR} ]; then
	echo "executed from the wrong dir"
	exit 1
fi

OLDPWD=`pwd`

cd ${DESTDIR} || exit 1
echo -n "generating md5..."
gen_md5
echo "done"
cd ${OLDPWD} || exit 1
