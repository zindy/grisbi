#!/bin/bash

# Warnings enabled
CFLAGS=""
#CFLAGS="-Wall"
#CFLAGS+=" -Wextra"

#CFLAGS+=" -Wbad-function-cast"
#CFLAGS+=" -Wcast-align"
#CFLAGS+=" -Wchar-subscripts"
#CFLAGS+=" -Wempty-body"
#CFLAGS+=" -Wformat"
#CFLAGS+=" -Wformat-security"
#CFLAGS+=" -Winit-self"
#CFLAGS+=" -Winline"
#CFLAGS+=" -Wmissing-declarations"
#CFLAGS+=" -Wmissing-include-dirs"
#CFLAGS+=" -Wmissing-prototypes"
#CFLAGS+=" -Wnested-externs"
#CFLAGS+=" -Wold-style-definition"
#CFLAGS+=" -Wpointer-arith"
#CFLAGS+=" -Wredundant-decls"
#CFLAGS+=" -Wshadow"
#CFLAGS+=" -Wstrict-prototypes"
#CFLAGS+=" -Wswitch-enum"
#CFLAGS+=" -Wundef"
#CFLAGS+=" -Wuninitialized"
#CFLAGS+=" -Wunused"
#CFLAGS+=" -Wwrite-strings"

# warnings disabled on purpose
CFLAGS+=" -Wno-unused-parameter"
#CFLAGS+=" -Wno-unused-function"
CFLAGS+=" -Wno-deprecated-declarations"

# fails on warning
#CFLAGS+=" -Werror"

export CFLAGS
echo "CFLAGS: $CFLAGS"

if [ "$TRAVIS_OS_NAME" = "osx" ]
then
	# from brew
	export PKG_CONFIG_PATH=/usr/local/Cellar/libxml2/*/lib/pkgconfig
	export PATH="$PATH:/usr/local/Cellar/gettext/*/bin"
fi

sh -x ./autogen.sh
./configure "$@"
