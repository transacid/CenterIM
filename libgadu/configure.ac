AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(libgadu, 0.1)

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX

AC_ARG_ENABLE(gg, [  --disable-gg   Build without Gadu-Gadu], build_gg="$enableval", build_gg="yes")
AM_CONDITIONAL(BUILD_GADU, test "x$build_gg" = xyes)

if test "$build_gg" = "yes"; then
    if test -n "$with_ssl" -a x"$with_ssl" != xno; then
        AC_DEFINE(HAVE_SSL)
        if test -n "$with_openssl" -a x"$with_openssl" != xno; then
            AC_DEFINE(HAVE_OPENSSL)
        fi
    fi

    AC_MSG_CHECKING(endianness)
    AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <sys/param.h>
    ], [
	#if defined(BYTE_ORDER) && defined(BIG_ENDIAN)
	    #if BYTE_ORDER == BIG_ENDIAN
		bogus
	    #endif
	#else
	    #if defined(_BIG_ENDIAN)
		bogus
	    #endif
	#endif
    ],[
	ac_end=little
    ],[
	AC_DEFINE(WORDS_BIGENDIAN, [big endian architecture])
	ac_end=big
    ])
    AC_MSG_RESULT($ac_end)

    AC_CHECK_HEADERS([stdint.h sys/types.h],,)
fi

AC_OUTPUT(Makefile)
