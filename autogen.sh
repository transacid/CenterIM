#! /bin/sh
# script shamelessy taken from openbox

sh() {
  /bin/sh -c "set -x; $*"
}

sh autopoint --force # for GNU gettext
sh libtoolize --copy --force --automake

# ok, this is wierd, but apparantly every subdir
# in the centericq sources has its own configure
# no idea why. should fix this.

TOP=$PWD
traverse=`find $PWD -name "configure.[ia][nc]" -print`
for i in $traverse; do
	echo Changing directory to `dirname $i`
	pushd `dirname $i` > /dev/null

	if test "$PWD" = "$TOP"; then
		sh aclocal -I m4 $ACLOCAL_FLAGS
	else
		sh aclocal $ACLOCAL_FLAGS
	fi
	headneeded=`grep -E "A(M|C)_CONFIG_HEADER" configure.[ia][nc]`
	if test ! -z "$headneeded"; then sh autoheader; fi
	sh autoconf
	sh automake --add-missing --copy

	popd > /dev/null
done

echo
echo You are now ready to run ./configure
echo enjoy!
