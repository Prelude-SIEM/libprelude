libtoolize -c --force --ltdl --automake
autoreconf --install --force
gtkdocize --copy || echo "gtkdocize missing. Please install gtk-doc."; exit 1

echo
echo "You can now run \"./configure --enable-developer-mode\" and \"make\""
