aclocal -I m4
autoconf
autoheader
libtoolize -c --force --ltdl --automake
automake --gnu -a -c
#./configure
