aclocal
autoconf
autoheader
libtoolize -c --force --ltdl --automake
automake --gnu -a -c
#./configure

#
# Libtool ship with an old configure.in,
# which may cause some problem, thus this hack :
#

cd libltdl

mv configure.in configure.in.tmp
echo "AC_PREREQ(2.50)" > configure.in
cat configure.in.tmp >> configure.in
rm -f configure.in.tmp

aclocal
autoconf
autoheader
automake

cd ..
