AC_INIT(aclocal.m4)
AM_INIT_AUTOMAKE(libicq2000, 0.1)

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_RANLIB

AC_STRUCT_TIMEZONE

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_CHECK_HEADERS([vector iostream fstream string algorithm list memory iterator exception utility],,
  AC_MSG_ERROR([You need to have the libstdc++ headers installed]))

AC_CHECK_HEADERS([hash_map strstream ext/hash_map sstream],,)

AC_CHECK_HEADERS([ext/hash_map],,
  [AC_CHECK_HEADERS(hash_map,,
    AC_MSG_ERROR([Your libstdc++ installation does not have the extra hash classes]))])

AC_CHECK_HEADERS(sstream,,
  [AC_CHECK_HEADERS(strstream,,
    AC_MSG_ERROR([Your libstdc++ doesn't have the sstream or strstream classes]))])

AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>
],[ socklen_t foo; ], ac_cv_c_socklen_t=yes, ac_cv_c_socklen_t=no)

if test $ac_cv_c_socklen_t = no; then
  AC_DEFINE(socklen_t, int, [Define type of socklen_t for systems missing it])
fi

AC_LANG_RESTORE

AC_OUTPUT(Makefile libicq2000/Makefile src/Makefile)
