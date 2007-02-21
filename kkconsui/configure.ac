AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(kkconsui, 0.1)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB


###	use option --enable-locales-fix to fix problems with double-byte
###	encodings on systems with broken locale, iscntrl()

AC_ARG_ENABLE(locales-fix,
	[  --enable-locales-fix    enables workaround for broken locales. use this
			  option if some of characters in your texts in Chinese
			  or Japanese diappear],
        [if test "$enableval" = "yes"; then
                AC_DEFINE(LOCALES_HACK)
        fi])


###	use option --with-fribidi to compile with fribidi support
AC_ARG_WITH(fribidi,
	[  --with-fribidi=[DIR]    enable Hebrew and Arabic support using fribidi
                          library in DIR],
	[with_fribidi=$withval],
	[with_fribidi=no])

if test "$with_fribidi" = "yes"; then
    for ac_dir in /usr/local /usr; do
        if test -f "$ac_dir/include/fribidi/fribidi.h"; then
            with_fribidi=$ac_dir
            break;
        fi
    done
fi

if test -n "$with_fribidi" -a "$with_fribidi" != "no"; then
    if test "$with_fribidi" = "yes"; then with_fribidi="/usr"; fi

    if test -r $with_fribidi/include/fribidi/fribidi.h; then
        echo "enabling FriBiDi support in $with_fribidi"
	CXXFLAGS="$CXXFLAGS `$with_fribidi/bin/fribidi-config --cflags`"
	LIBS="$LIBS `$with_fribidi/bin/fribidi-config --libs`"
    fi

    AC_DEFINE(KTOOL_USE_FRIBIDI)
else
    echo 'disabling FriBiDi support...'
fi

case "$host_os" in
	darwin*)
	    CXXFLAGS="$CXXFLAGS -I/sw/include"
	;;
esac

AC_CHECK_HEADERS(_G_config.h)

AC_OUTPUT(Makefile include/Makefile src/Makefile)
