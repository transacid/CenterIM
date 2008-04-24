#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(kkstrtext, 0.1)
AC_CONFIG_SRCDIR([kkstrtext.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
AC_GNU_SOURCE
AC_LANG_CPLUSPLUS

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.16.1])

# Checks for libraries.
AM_ICONV

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h _G_config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([strchr strdup strncasecmp strpbrk strspn strstr strtol])

#Output Makefile
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
