libtoolize -c --force --ltdl --automake
autoreconf --install --force
gtkdocize

echo
echo "You can now run \"./configure --enable-developer-mode\" and \"make\""
