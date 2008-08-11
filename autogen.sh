#!/bin/sh

GNULIB_TOOL=`which gnulib-tool 2> /dev/null`
if test -n "$GNULIB_TOOL"; then
	echo "Running gnulib-tool..."
        $GNULIB_TOOL --import > /dev/null || exit 1
fi

GTKDOCIZE=`which gtkdocize 2> /dev/null`
if test -n "$GTKDOCIZE"; then
	echo "Running gtkdocize..."
	$GTKDOCIZE --copy || exit 1
fi

echo "Running libtoolize..."
libtoolize -c --force --ltdl --automake || exit 1

echo "Running autoreconf..."
autoreconf --install --force || exit 1

echo
echo "You can now run \"./configure --enable-developer-mode\" and \"make\""
