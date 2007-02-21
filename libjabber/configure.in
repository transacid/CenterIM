AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(libjabber, 0.1)

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX

AC_ARG_ENABLE(jabber, [  --disable-jabber   Build without Jabber], build_jabber="$enableval", build_jabber="yes")
AM_CONDITIONAL(BUILD_JABBER, test "x$build_jabber" = xyes)

AC_OUTPUT(Makefile)
