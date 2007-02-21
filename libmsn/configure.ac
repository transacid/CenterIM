AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(libmsn, 0.1)

AM_CONFIG_HEADER(config.h)

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_RANLIB

AC_ARG_ENABLE(msn, [  --disable-msn   Build without MSN], build_msn="$enableval", build_msn="yes")
AM_CONDITIONAL(BUILD_MSN, test "x$build_msn" = xyes)

if test "$build_msn" = "yes"; then
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_CHECK_HEADERS([sstream strstream])
    AC_LANG_RESTORE

    # Checks for header files.
    AC_HEADER_STDC
    AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h])

    # Checks for typedefs, structures, and compiler characteristics.
    AC_C_CONST
    AC_TYPE_SIZE_T
    AC_HEADER_TIME

    # Checks for library functions.
    AC_FUNC_SELECT_ARGTYPES
    AC_CHECK_FUNCS([gethostbyname gethostname getpass inet_ntoa memset select socket strerror strtol])

    # Checks for libraries.
    AC_ARG_WITH(curl,
	[  --with-curl=[DIR]    curl-config in DIR],
	[CURL_CONFIG=$withval],
	[ AC_PATH_PROG(CURL_CONFIG,
	    curl-config,
	    AC_MSG_ERROR(curl-config not found))])

    CXXFLAGS="$CXXFLAGS `$CURL_CONFIG --cflags`"
fi

AC_OUTPUT(Makefile msn/Makefile)
