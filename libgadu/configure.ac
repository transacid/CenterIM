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
	else
	    AC_MSG_RESULT([not found or disabled])
	    with_ssl="no"

	    AM_PATH_LIBGNUTLS_EXTRA(0.0.1, [
	    AC_CHECK_LIB(lzo, lzo1x_1_compress, LZO_LIBS=-llzo)
	    if test "$LZO_LIBS" = ""; then
	     AC_CHECK_LIB(lzo2, lzo1x_1_compress, LZO_LIBS=-llzo2, [
	      AC_MSG_ERROR(
***
*** Could not find liblzo or liblzo2.
)
])
            fi
	                 CXXFLAGS="$CXXFLAGS $LIBGNUTLS_CFLAGS"
	                 LIBS="$LIBS $LIBGNUTLS_LIBS -lgnutls-extra"
	                 AC_DEFINE(HAVE_GNUTLS, 1, [use gnutls])
	    with_ssl="yes"
	    ac_configure_args="$ac_configure_args --with-ssl"
	    ])
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
