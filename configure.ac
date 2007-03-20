#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([centerim], [4.22.0], [http://centericq.de/mailinglist.php])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE
AC_ISC_POSIX

AC_CANONICAL_HOST

AC_GNU_SOURCE

########################
# Checks for programs. #
########################
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.13.1])
AC_PROG_YACC
AC_PROG_GCC_TRADITIONAL

#########################
# Checks for libraries. #
#########################
AC_ARG_ENABLE(yahoo,
              AS_HELP_STRING([--disable-yahoo], [Build without Yahoo!]),
              build_yahoo="$enableval",
              build_yahoo="yes"
              )
AC_ARG_ENABLE(aim,
              AS_HELP_STRING([--disable-aim], [Build without AIM]),
              build_aim="$enableval",
              build_aim="yes"
              )
AC_ARG_ENABLE(irc,
              AS_HELP_STRING([--disable-irc], [Build without IRC]),
              build_irc="$enableval",
              build_irc="yes"
              )
AC_ARG_ENABLE(jabber,
              AS_HELP_STRING([--disable-jabber], [Build without Jabber]),
              build_jabber="$enableval",
              build_jabber="yes"
              )
AC_ARG_ENABLE(gg,
              AS_HELP_STRING([--disable-gg], [Build without Gadu-Gadu]),
              build_gg="$enableval",
              build_gg="yes"
              )
AC_ARG_ENABLE(msn,
              AS_HELP_STRING([--disable-msn], [Build without MSN]),
              build_msn="$enableval",
              build_msn="yes"
              )
AC_ARG_ENABLE(rss,
              AS_HELP_STRING([--disable-rss], [Build without RSS reader]),
              build_rss="$enableval",
              build_rss="yes"
              )
AC_ARG_ENABLE(lj,
              AS_HELP_STRING([--disable-lj], [Build without LiveJournal client]),
              build_lj="$enableval",
              build_lj="yes"
              )
AC_ARG_ENABLE(locales-fix,
              AS_HELP_STRING([--enable-locales-fix], [enables workaround for
                              broken locales. use this option if some of
                              characters in your texts in Chinese or Japanese
                              diappear])
              )

AC_ARG_WITH(fribidi,
            AS_HELP_STRING([--with-fribidi=[DIR]], [enable Hebrew and Arabic
                            support using fribidi library in DIR]),
            [with_fribidi=$withval],
            [with_fribidi=no]
            )
AC_ARG_WITH(ssl,
            AS_HELP_STRING([--with-ssl], [enable SSL secured connections using
                            either OpenSSL or GnuTLS]),
            [with_ssl=$withval],
            [with_ssl=no]
            )
AC_ARG_WITH(openssl,
            AS_HELP_STRING([--with-openssl=[DIR]], [enable SSL secured
                            connections using the OpenSSL library in DIR
                            (optional)]),
            [with_openssl=$withval],
            [with_openssl=no]
            )
AC_ARG_WITH(gpgme,
            AS_HELP_STRING([--with-gpgme=[PATH]], [path to gpgme-config (auto)]),
            [with_gpgme=$withval],
            [with_gpgme=no]
            )
AC_ARG_WITH(curl,
            AS_HELP_STRING([--with-curl=[PATH]], [path to curl-config (auto)]),
            [with_curl=$withval],
            [with_curl=no]
            )
AC_ARG_WITH(libjpeg,
            AS_HELP_STRING([--without-libjpeg], [Compile without JPEG token
                            support (for Gadu-Gadu registration)])
            )

#####################
# Check for fribidi #
#####################
AC_MSG_CHECKING(whether to use fribidi)
FRIBIDI_LIBS=""
FRIBIDI_CFLAGS=""
FRIBIDI_PC="" 
have_fribidi=no 

if test "x$with_fribidi" = "xyes"; then
	AC_PATH_PROG(FRIBIDI_CONFIG, fribidi-config, no)
	if test x$FRIBIDI_CONFIG = xno; then
		AC_MSG_ERROR([*** fribidi-config not found])
	fi
	
	FRIBIDI_CFLAGS="`$FRIBIDI_CONFIG --cflags`"
	CFLAGS="$CFLAGS $FRIBIDI_CFLAGS"
	FRIBIDI_LIBS="`$FRIBIDI_CONFIG --libs`"
	FRIBIDI_PC="fribidi,"

	AC_CHECK_LIB(fribidi, fribidi_get_mirror_char, :, AC_MSG_ERROR([*** libfribidi not found.]), $FRIBIDI_LIBS)
	AC_DEFINE(HAVE_FRIBIDI, 1, [has fribidi library])
	have_fribidi=yes
fi
AC_MSG_RESULT([$have_fribidi])

#################
# Check for SSL #
#################
AC_MSG_CHECKING(for SSL)
AC_MSG_RESULT([])

if test "$with_ssl" != "no"; then

	AC_MSG_CHECKING(for OpenSSL)
	if test -z "$with_openssl"; then
		for ac_dir in $with_openssl /usr/local /usr; do
			if test -f "$ac_dir/include/openssl/ssl.h"; then
				with_openssl=$ac_dir
				break;
			fi
		done
	fi

	if test -n "$with_openssl" -a "$with_openssl" != "no"; then
		if test "$with_openssl" = "yes"; then 
			with_openssl="/usr"
		fi
		AC_MSG_RESULT([found in $with_openssl])
                CFLAGS="$CFLAGS -I${with_openssl}"
		AC_DEFINE(HAVE_OPENSSL, 1, [use openssl])
		AC_CHECK_LIB(crypto, main)
		AC_CHECK_LIB(ssl, main)
	else
		AC_MSG_RESULT([not found or disabled])
	        with_ssl="no"

		AM_PATH_LIBGNUTLS_EXTRA(0.0.1, [
			CXXFLAGS="$CXXFLAGS $LIBGNUTLS_CFLAGS"
			LIBS="$LIBS $LIBGNUTLS_LIBS -lgnutls-extra"
			AC_DEFINE(HAVE_GNUTLS, 1, [use gnutls]) with_ssl="yes" ])
	fi
else 
	AC_MSG_RESULT([disabled (use --with-ssl=[DIR] to enable)])
fi

###################
# Check for gpgme #
###################
GPGME_CFLAGS=""
GPGME_CXXFLAGS=""
GPGME_LIBS=""
have_gpgme=no

if test x"$with_gpgme" = x"no"; then
    AC_MSG_CHECKING(for gpgme)

	if test -x "$with_gpgme"; then
		GPGME_CONFIG= "$with_gpgme"
	else
		AC_PATH_PROG(GPGME_CONFIG, gpgme-config, no)
	fi

	if test x$GPGME_CONFIG = xno; then
        AC_MSG_RESULT([no])
    else
        AC_MSG_RESULT([yes])
	fi

	save_CFLAGS="$CFLAGS"
	save_CXXFLAGS="$CXXFLAGS"
	save_LIBS="$LIBS"

	GPGME_CFLAGS="`$GPGME_CONFIG --cflags`"
	GPGME_CXXFLAGS="`$GPGME_CONFIG --cflags`"
	GPGME_LIBS="`$GPGME_CONFIG --libs`"
	CFLAGS="$CFLAGS $GPGME_CFLAGS"
	CXXFLAGS="$CXXFLAGS $GPGME_CXXFLAGS"
	LIBS="$LIBS $GPGME_LIBS"

	AC_MSG_CHECKING(for the OpenPGP protocol in gpgme)
	AC_TRY_RUN([
		#include <gpgme.h>
		int main() {
			gpgme_ctx_t ctx;
			if(!gpgme_new(&ctx)) if(!gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP)) return 0;
			return 1;
		}
	], opengpg_avail=yes, opengpg_avail=no, opengpg_avail=no)
	AC_MSG_RESULT([$opengpg_avail])

	if test "$opengpg_avail" != "yes"; then
		CFLAGS="$save_CFLAGS"
		CXXFLAGS="$save_CXXFLAGS"
		LIBS="$save_LIBS"
	fi
	AC_DEFINE(HAVE_GPGME, 1, [has gpgme library])
        have_gpgme=yes
fi

##################
# Check for curl #
##################
AC_MSG_CHECKING(for cURL)
AC_MSG_RESULT([])
CURL_CFLAGS=""
CURL_CXXFLAGS=""
CURL_LIBS=""
have_curl=no

if true; then
        if test -x "$with_curl"; then
                CURL_CONFIG= "$with_curl"
        else
                AC_PATH_PROG(CURL_CONFIG, curl-config, no)
        fi

        if test x$CURL_CONFIG = xno; then
                AC_MSG_ERROR([*** curl-config not found])
        fi

        save_CFLAGS="$CFLAGS"
        save_CXXFLAGS="$CXXFLAGS"
        save_LIBS="$LIBS"

        CURL_CFLAGS="`$CURL_CONFIG --cflags`"
        CURL_CXXFLAGS="`$CURL_CONFIG --cflags`"
        CURL_LIBS="`$CURL_CONFIG --libs`"
        CFLAGS="$CFLAGS $CURL_CFLAGS"
        CXXFLAGS="$CXXFLAGS $CURL_CXXFLAGS"
        LIBS="$LIBS $CURL_LIBS"

        AC_MSG_CHECKING(for SSL support in libcurl)
        curlssl=`$CURL_CONFIG --feature | grep SSL`
        if test -z "$curlssl"; then
		curlssl=no
		AC_MSG_RESULT([no])
        else
		curlssl=yes
		AC_MSG_RESULT([yes])
        fi

        AC_DEFINE(HAVE_CURL, 1, [has curl library])
        have_curl=yes
fi

#####################
# Check for libjpeg #
#####################
AC_MSG_CHECKING(for libjpeg (for Gadu-Gadu registration))
JPEG_LIBS=""
have_libjpeg=no

if test "x$with_libjpeg" != "xno"; then
	AC_MSG_RESULT([])

	AC_CHECK_LIB(jpeg, jpeg_start_decompress,
	[
		AC_CHECK_HEADERS(jpeglib.h,
		[
			AC_DEFINE(HAVE_LIBJPEG, 1, [define if you have libjpeg])
			JPEG_LIBS="-ljpeg $LIBS"
			LIBS="$LIBS $JPEG_LIBS"
			have_libjpeg=yes
		])
	])
fi

####################
# Check for others #
####################
AM_ICONV_LINK
LIBS="$LIBS $LIBICONV"

AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(ncurses, initscr,, [AC_CHECK_LIB(curses, initscr,, AC_MSG_ERROR(The ncurses terminal library is required in order to build the program))])

AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>
],[ socklen_t foo; ], ac_cv_c_socklen_t=yes, ac_cv_c_socklen_t=no)

if test "$ac_cv_c_socklen_t" = "no"; then
  AC_DEFINE(socklen_t, int, [Define type of socklen_t for systems missing it])
fi

case "$host_os" in
	darwin*)
		LIBS="$LIBS -flat_namespace"
		;;
esac

AM_CONDITIONAL(BUILD_YAHOO, test "x$build_yahoo" = xyes)
AM_CONDITIONAL(BUILD_AIM, test "x$build_aim" = xyes)
AM_CONDITIONAL(BUILD_IRC, test "x$build_irc" = xyes)
AM_CONDITIONAL(BUILD_FIRETALK, test "x$build_irc" = xyes -o "x$build_aim" = xyes)
AM_CONDITIONAL(BUILD_JABBER, test "x$build_jabber" = xyes)
AM_CONDITIONAL(BUILD_GADU, test "x$build_gg" = xyes)
AM_CONDITIONAL(BUILD_MSN, test "x$build_msn" = xyes)
AM_CONDITIONAL(BUILD_RSS, test "x$build_rss" = xyes -o "x$build_lj" = xyes)
AM_CONDITIONAL(BUILD_LJ, test "x$build_lj" = xyes)
AM_CONDITIONAL(HAVE_FRIBIDI, test "x$have_fribidi" = xyes)

if test "$build_yahoo" = "yes"; then
	AC_DEFINE(BUILD_YAHOO, 1, [build with yahoo support])
fi
if test "$build_aim" = "yes"; then
	AC_DEFINE(BUILD_AIM, 1, [build with aim support])
fi
if test "$build_irc" = "yes";
	then AC_DEFINE(BUILD_IRC, 1, [build with irc support])
fi
if test "$build_jabber" = "yes"; then
	if test $have_gpgme = no; then
        	AC_MSG_WARN([No GPG support in Jabber, since GPGME library not found or its setup not ok])
	fi
	AC_DEFINE(BUILD_JABBER, 1, [build with jabber support])
fi
if test "$build_gg" = "yes"; then
	if test $have_libjpeg = no; then
        	AC_MSG_WARN([No jpeg support in Gadu-Gadu, you will not be able to register??????????])
	fi
	AC_DEFINE(BUILD_GADU, 1, [build with gadu-gadu support])
fi
if test "$build_msn" = "yes"; then
	if test $have_curl = no; then
		AC_MSG_ERROR([MSN cannot be built without the curl library.])
	fi
	if test $curlssl = no; then
		AC_MSG_ERROR([MSN requires libcurl built with SSL support.])
	fi
	AC_DEFINE(BUILD_MSN, 1, [build with MSN support])
fi
if test "$build_rss" = "yes" -o "$build_lj" = "yes"; then
	AC_DEFINE(BUILD_RSS, 1, [build the integrated rss feeds reader])
fi
if test "$build_lj" = "yes"; then
	AC_DEFINE(BUILD_LJ, 1, [build the integrated livejournal client])
fi

############################
# Checks for header files. #
############################
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([argz.h arpa/inet.h fcntl.h float.h inttypes.h langinfo.h libintl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdio_ext.h stdlib.h string.h strings.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h sys/vfs.h syslog.h unistd.h utmp.h wchar.h])
AC_CHECK_HEADERS([stdint.h sys/types.h],,)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_CHECK_HEADERS([vector iostream fstream string algorithm list memory iterator exception utility],, AC_MSG_ERROR([You need to have the libstdc++ headers installed]))
AC_CHECK_HEADERS([hash_map strstream ext/hash_map sstream],,)
AC_LANG_RESTORE

##################################################################
# Checks for typedefs, structures, and compiler characteristics. #
##################################################################
AC_STRUCT_TIMEZONE
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])
AC_TYPE_SIGNAL

#################
# Substitutions #
#################
AC_SUBST(FRIBIDI_LIBS)
AC_SUBST(FRIBIDI_CFLAGS) 
AC_SUBST(FRIBIDI_PC) 

AC_SUBST(GPGME_LIBS)
AC_SUBST(GPGME_CFLAGS) 
AC_SUBST(GPGME_CXXFLAGS) 

AC_SUBST(CURL_LIBS)
AC_SUBST(CURL_CFLAGS) 
AC_SUBST(CURL_CXXFLAGS) 

AC_SUBST(JPEG_LIBS)

#################################
# Checks for library functions. #
#################################
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_FUNC_WAIT3                                  
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify alarm atexit bzero dup2 floor getcwd gethostbyname gethostname gettimeofday inet_ntoa isascii memmove mempcpy memset mkdir modf munmap nl_langinfo putenv regcomp rmdir select setenv setlocale socket stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol strtoul uname])

########################
# Create output files. #
########################
# also configure other packages
AC_CONFIG_SUBDIRS(connwrap firetalk kkconsui kkstrtext kksystr libgadu libicq2000 libjabber libmsn libyahoo2)

ALL_LINGUAS="zh_TW.Big5 uk sv ru ro pt_BR pl nl ms it hu fr es de cs bg"
AC_CONFIG_FILES([Makefile
                 misc/Makefile
                 share/Makefile
                 src/Makefile
                 src/hooks/Makefile
                 intl/Makefile
                 po/Makefile.in])

AC_OUTPUT

AC_MSG_RESULT([
Build options:
  Version               $VERSION
  Install prefix        $prefix
  Build shared libs     $enable_shared
  Build static libs     $enable_static
  CFLAGS                $CFLAGS
  CPPFLAGS              $CPPFLAGS

Protocols and Clients:])
if test "x$build_yahoo" = xyes; then AC_MSG_RESULT([  Yahoo]); fi
if test "x$build_aim" = xyes; then AC_MSG_RESULT([  AIM]); fi
if test "x$build_jabber" = xyes; then AC_MSG_RESULT([  jabber]); fi
if test "x$build_gg" = xyes; then AC_MSG_RESULT([  Gadu-Gadu]); fi
if test "x$build_msn" = xyes; then AC_MSG_RESULT([  MSN Messenger]); fi
if test "x$build_rss" = xyes; then AC_MSG_RESULT([  RSS Reader]); fi
if test "x$build_lj" = xyes; then AC_MSG_RESULT([  LiveJournal client]); fi
if test "x$have_fribidi" = xyes; then AC_MSG_RESULT([  LiveJournal client]); fi
AC_MSG_RESULT([
Enjoy!])
