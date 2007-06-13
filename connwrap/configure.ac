AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(connwrap, 0.1)

AC_PROG_RANLIB

AC_PROG_CC
AC_PROG_CXX

###
###     SSL libs
###

if test -n "$with_ssl" -a x"$with_ssl" != xno; then
    AC_DEFINE(HAVE_SSL)
    if test -n "$with_openssl" -a x"$with_openssl" != xno; then
        AC_DEFINE(HAVE_OPENSSL)
        AC_CHECK_LIB(crypto, main, [
            AC_CHECK_LIB(crypto, SSLeay_add_all_algorithms, [
                AC_DEFINE(HAVE_SSLEAY)
            ])
            ])
     else
         AC_MSG_RESULT([not found or disabled])
         with_ssl="no"

          AM_PATH_LIBGNUTLS_EXTRA(0.0.1, [
          AC_CHECK_LIB(lzo, lzo1x_1_compress, [], [
                            echo "lzo compression is required.";
			    exit -1
			    ] )
		       CXXFLAGS="$CXXFLAGS $LIBGNUTLS_CFLAGS"
		       LIBS="$LIBS $LIBGNUTLS_LIBS -lgnutls-extra"
		       AC_DEFINE(HAVE_GNUTLS, 1, [use gnutls])
          with_ssl="yes"
          ac_configure_args="$ac_configure_args --with-ssl"
	  ])
    fi
fi

AC_MSG_CHECKING(for inet_aton() presence)

AC_TRY_LINK([

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

], [

struct in_addr inp;
inet_aton("address", &inp);

], have_aton=yes, have_aton=no)

if test "$have_aton" = "yes"; then
    AC_DEFINE(HAVE_INET_ATON)
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi

AC_OUTPUT(Makefile)
