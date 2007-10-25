# Copyright IBM Corp. 2007
libtoolize --copy --force --automake
aclocal --force
autoheader --force
automake -i --add-missing --copy --foreign
autoconf --force
echo "You may now run ./configure"
