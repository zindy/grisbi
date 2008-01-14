#!/bin/sh
#
# autogen.sh glue for Grisbi
# $Id: autogen.sh,v 1.11 2008/01/14 00:21:06 mpupat Exp $
#
# Requires: automake, autoconf, dpkg-dev

PATH_AUTOMAKE=/usr/share/automake

# test for some distribution...
# Y'a pas plus simple ?

if test -x /usr/share/automake-1.10
then
	PATH_AUTOMAKE=/usr/share/automake-1.10
fi

if test -x /usr/share/automake-1.9
then
	PATH_AUTOMAKE=/usr/share/automake-1.9
fi

if test -x /usr/share/automake-1.8
then
	PATH_AUTOMAKE=/usr/share/automake-1.8
fi

if test -x /usr/share/automake-1.7
then
	PATH_AUTOMAKE=/usr/share/automake-1.7
fi

if test -x /usr/share/automake-1.6
then
	PATH_AUTOMAKE=/usr/share/automake-1.6
fi

if test -x /usr/share/automake-1.4
then
	PATH_AUTOMAKE=/usr/share/automake-1.4
fi

# Refresh GNU autotools toolchain.
for i in config.guess config.sub missing install-sh mkinstalldirs ; do
	test -r $PATH_AUTOMAKE/${i} && {
		rm -f ${i}
		cp $PATH_AUTOMAKE/${i} .
	}
	if test -r ${i} ; then
	    chmod 755 ${i}
	fi
done



#
# Apple's Developer Tools have a "libtool" that has nothing to do with
# the GNU libtool; they call the latter "glibtool".  They also call
# libtoolize "glibtoolize".
#
# Check for "glibtool" first.
# Borrowed from ethereal
#
LTVER=`glibtool --version 2>/dev/null | grep ' libtool)' | \
    sed 's/.*libtool) \([0-9][0-9.]*\)[^ ]* .*/\1/'`
if test -z "$LTVER"
then
	LTVER=`libtool --version | grep ' libtool)' | \
	    sed 's/.*) \([0-9][0-9.]*\)[^ ]* .*/\1/' `
	LIBTOOLIZE=libtoolize
else
	LIBTOOLIZE=glibtoolize
fi
case "$LTVER" in
'' | 0.* | 1.[0-3]* )

  cat >&2 <<_EOF_

	You must have libtool 1.4 or later installed to compile $PROJECT.
	Download the appropriate package for your distribution/OS,
	or get the source tarball at ftp://ftp.gnu.org/pub/gnu/libtool/
_EOF_
  exit 1
  ;;
esac

LTARGS=" --copy --force"
$LIBTOOLIZE $LTARGS


aclocal -I macros
autoheader
automake --verbose --foreign --add-missing
autoconf

exit 0
