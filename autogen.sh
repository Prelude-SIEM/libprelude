libtoolize -c --force --ltdl --automake
autoreconf --install
echo
echo "You can now run \"./configure --enable-developer-mode\" and \"make\""
