# $OpenBSD$

COMMENT =	cool curses irc client

DISTNAME =	swirc-2.8.0

GH_ACCOUNT =	uhlin
GH_PROJECT =	swirc
GH_TAGNAME =	v2.8.0

CATEGORIES =	net

HOMEPAGE =	https://www.nifty-networks.net/swirc/

MAINTAINER =		Markus Uhlin <markus.uhlin@bredband.net>

# BSD-3, ISC and MIT.
PERMIT_PACKAGE_CDROM =	Yes

# uses pledge()

WANTLIB =		c crypto curl curses m panel ssl

MASTER_SITES =		https://www.nifty-networks.net/swirc/releases/

EXTRACT_SUFX =		.tgz

CONFIGURE_STYLE =	simple

ALL_TARGET =		all
INSTALL_TARGET =	install

.include <bsd.port.mk>
