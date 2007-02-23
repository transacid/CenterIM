AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(firetalk, 0.1)

AC_PROG_RANLIB
AC_PROG_CC

AC_ARG_ENABLE(aim, [  --disable-aim   Build without AIM], build_aim="$enableval", build_aim="yes")
AC_ARG_ENABLE(irc, [  --disable-irc   Build without IRC], build_irc="$enableval", build_irc="yes")

AM_CONDITIONAL(BUILD_FIRETALK, test "x$build_aim" = xyes -o "x$build_irc" = xyes)

if test "$build_aim" = "yes" -o "$build_irc" = "yes"; then
    AC_PROG_CXX

    AC_CHECK_HEADERS(stdint.h)

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
fi

AC_OUTPUT(Makefile)
