# $OpenBSD$

COMMENT =	curses icb and irc client
DISTNAME =	swirc-3.0.0
CATEGORIES =	net
HOMEPAGE =	https://www.nifty-networks.net/swirc/

MAINTAINER =		Markus Uhlin <markus.uhlin@bredband.net>

# BSD-3, ISC and MIT.
PERMIT_PACKAGE =	Yes

# uses pledge()
WANTLIB =		${COMPILER_LIBCXX} c crypto curl curses m panel pthread ssl

MASTER_SITES =		https://www.nifty-networks.net/swirc/releases/
EXTRACT_SUFX =		.tgz
LIB_DEPENDS =		net/curl
CONFIGURE_STYLE =	simple

.include <bsd.port.mk>
