aclocal
libtoolize -c --force --ltdl --automake
autoheader
automake --gnu -a -c
autoconf
#./configure
