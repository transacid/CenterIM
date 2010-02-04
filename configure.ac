#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([centerim], m4_esyscmd([misc/git-version-gen .tarball-version]), [http://bugzilla.centerim.org])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall])

AC_ISC_POSIX

AC_CANONICAL_HOST

AC_GNU_SOURCE

########################
# Checks for programs. #
########################
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_RANLIB
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.14.6])


#########################
# Checks for libraries. #
#########################
AC_ARG_ENABLE(icq,
              AS_HELP_STRING([--disable-icq], [Build without ICQ]),
              build_icq="$enableval",
              build_icq="yes"
              )
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
                              disappear]),
              locales_fix="$enableval",
              locales_fix="yes"
              )

AC_ARG_WITH(fribidi,
            AS_HELP_STRING([--with-fribidi=[DIR]], [enable Hebrew and Arabic
                            support using fribidi library in DIR]),
            [with_fribidi=$withval],
            [with_fribidi=no]
            )
AC_ARG_WITH(ssl,
            AS_HELP_STRING([--with-ssl], [enable SSL secured connections using
                            either OpenSSL or GnuTLS (auto)]),
            [with_ssl=$withval]
            )
AC_ARG_WITH(openssl,
            AS_HELP_STRING([--with-openssl=[DIR]], [enable SSL secured
                            connections using the OpenSSL library in DIR
                            (optional)]),
            [with_openssl=$withval]
            )
AC_ARG_WITH(nss_compat,
            AS_HELP_STRING([--with-nss_compat=[DIR]], [enable SSL secured
                            connections using the NSS compat library in DIR
                            (optional)]),
            [with_nss_compat=$withval]
            )
AM_PATH_GPGME(
            [0.4.2],
            [have_gpgme=yes],
            [have_gpgme=no]
            )
LIBCURL_CHECK_CONFIG(
            [yes],
            [4.12.0],
            [have_libcurl=yes],
            [have_libcurl=no]
            )
AC_ARG_WITH(libjpeg,
            AS_HELP_STRING([--without-libjpeg], [Compile without JPEG token
                            support (for Gadu-Gadu registration)])
            )
AC_ARG_WITH(libotr,
            AS_HELP_STRING([--with-libotr], [enable Off-the-Record Messaging (libotr) WARNING: use at your own risk, not supported yet]),
            [with_libotr=$withval],
            [with_libotr=no]
            )
AC_ARG_WITH(libXss,
            AS_HELP_STRING([--with-libXss], [enable Xss]),
            [with_libXss=$withval],
            [with_libXss=no],
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
	if ! pkg-config fribidi; then
		AC_MSG_ERROR([*** fribidi.pc not found])
	fi
	
	FRIBIDI_CFLAGS=`pkg-config --cflags fribidi`
	CFLAGS="$CFLAGS $FRIBIDI_CFLAGS"
	FRIBIDI_LIBS=`pkg-config --libs fribidi`
	FRIBIDI_PC="fribidi,"

	AC_CHECK_LIB(fribidi, fribidi_get_mirror_char, :, AC_MSG_ERROR([*** libfribidi not found.]), $FRIBIDI_LIBS)
	AC_DEFINE(HAVE_FRIBIDI, 1, [has fribidi library])
    AC_DEFINE(KTOOL_USE_FRIBIDI, 1, [use fribidi in ktools])
	have_fribidi=yes
fi
AC_MSG_RESULT([$have_fribidi])

#################
# Check for SSL #
#################

if test "x$with_ssl" != "xno"; then

        with_ssl="no"

        AC_MSG_CHECKING(for OpenSSL)
        if test -z "$with_openssl" -o "x$with_openssl" = "xyes"; then
                for ac_dir in $with_openssl /usr/local /usr; do
                        if test -f "$ac_dir/include/openssl/ssl.h"; then
                                with_openssl=$ac_dir
                                break;
                        fi
                done
        fi

        if test -n "$with_openssl" -a "x$with_openssl" != "xno" -a -z "$with_nss_compat"; then
                CFLAGS="$CFLAGS -I${with_openssl}"

                AC_MSG_RESULT([found in $with_openssl])
                AC_DEFINE(HAVE_OPENSSL, 1, [use openssl])
                with_ssl="yes"

                AC_CHECK_LIB(crypto, main)
                AC_CHECK_LIB(ssl, main)

                AC_CHECK_LIB(crypto, SSLeay_add_all_algorithms,
                        AC_DEFINE(HAVE_SSLEAY, 1, [use SSLeay]))
        else
                AC_MSG_RESULT([not found or disabled])
        fi

        if test "x$with_ssl" = "xno" -a -z "$with_nss_compat"; then
                AC_CHECK_LIB(gnutls-extra, gnutls_global_init_extra, [
                        AC_CHECK_LIB(lzo, lzo1x_1_compress, LZO_LIBS=-llzo)

                        if test -z "$LZO_LIBS"; then
                                 AC_CHECK_LIB(lzo2, lzo1x_1_compress, LZO_LIBS=-llzo2, [
                                         AC_MSG_ERROR(
***
*** Could not find liblzo or liblzo2.
)
                                 ])
                        fi

                        CXXFLAGS="$CXXFLAGS $LIBGNUTLS_CFLAGS"
                        LIBS="$LIBS $LIBGNUTLS_LIBS -lgnutls-extra -lgnutls"
                        AC_DEFINE(HAVE_GNUTLS, 1, [use gnutls])
                        with_ssl="yes"
                ], AC_MSG_ERROR(
Could not find gnutls-extra. Stopping.
      ))
        fi

        if test "x$with_ssl" = "xno"; then
                PKG_PROG_PKG_CONFIG

		# NSS is essential for OpenSSL compatibility layer
		# drags in NSPR as well
                PKG_CHECK_MODULES(NSS, nss >= 3)
		CFLAGS="$CFLAGS $NSS_CFLAGS"
		CXXFLAGS="$CXXFLAGS $NSS_CFLAGS"
		LIBS="$LIBS $NSS_LIBS"

		# The compatibility layer
                AC_MSG_CHECKING(for NSS compat)
                if test -n "$with_nss_compat" -a "x$with_nss_compat" != "xno"; then
                        if test -f "${with_nss_compat}"; then
                                CFLAGS="$CFLAGS -I${with_nss_compat} -I/usr/include/nspr4"
                                CXXFLAGS="$CXXFLAGS -I${with_nss_compat} -I/usr/include/nspr4"
                        fi
                        AC_CHECK_LIB(nss_compat_ossl, SSL_CTX_new, [
                                AC_DEFINE(HAVE_NSS_COMPAT, 1, [use nss_compat])
                                LIBS="$LIBS -lnss_compat_ossl"
                                with_ssl="yes"
                        ])
                else
                        AC_MSG_RESULT([Not enabled, sadly])
                fi
        fi

        if test x"$with_ssl" != x"no"; then
                 AC_DEFINE(HAVE_SSL, 1, [ssl is available])
        else
                 AC_MSG_ERROR([Karamba! SSL library not found.])
        fi

else 
        AC_MSG_RESULT([disabled (use --with-ssl=[DIR] to enable)])
fi

###################
# Check for gpgme #
###################
if test "x$have_gpgme" = "xyes"; then
         AC_DEFINE(HAVE_GPGME, 1, [has gpgme library])
         GPGME_CXXFLAGS="$GPGME_CFLAGS"
         CFLAGS="$CFLAGS $GPGME_CFLAGS"
         CXXFLAGS="$CXXFLAGS $GPGME_CXXFLAGS"
         LIBS="$LIBS $GPGME_LIBS"
fi

###############################
# Check for Largefile support #
###############################
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_TYPE_OFF_T

#############################
# Check for pthread library #
#############################
original_LIBS="$LIBS"
LIBS="-lpthread $original_LIBS"

AC_TRY_LINK([#include<pthread.h>],
	    [pthread_create((void *)0, (void *)0, (void *)0, (void *)0)],
	    [pthread=yes], [pthread=no])

if test x${pthread} = xyes; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(HAVE_THREAD, 1, [pthread library is available])
else
	LIBS="$original_LIBS"
	AC_MSG_RESULT([no])
fi

AC_CHECK_HEADERS(sys/utsname.h, 
		 [
		  AC_CHECK_FUNC(uname, AC_DEFINE(HAVE_UNAME, 1, [check uname function]),)
		 ])
 
#####################
# Check for libcurl #
#####################

CURL_CFLAGS="$LIBCURL_CPPFLAGS"
CURL_CXXFLAGS="$LIBCURL_CPPFLAGS"
CURL_LIBS="$LIBCURL"
CFLAGS="$CFLAGS $CURL_CFLAGS"
CXXFLAGS="$CXXFLAGS $CURL_CXXFLAGS"
LIBS="$LIBS $CURL_LIBS"

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


#####################
# Check for libotr #
#####################
AC_MSG_CHECKING(for libotr)
have_libotr=no

if test "x$with_libotr" != "xno"; then
	AC_MSG_RESULT([])

	AC_CHECK_LIB(otr, otrl_init,
	[
		AC_CHECK_HEADERS(libotr/context.h libotr/userstate.h,
		[
			AC_DEFINE(HAVE_LIBOTR, 1, [define if you have libotr])
			OTR_LIBS="-lotr $LIBS"
			LIBS="$LIBS $OTR_LIBS"
			have_libotr=yes
		])
	])
fi

#####################
# Check for Xss     #
#####################
AC_MSG_CHECKING(whether to use Xss)
have_libXss=no 

if test "x$with_libXss" = "xyes"; then
	AC_MSG_RESULT([])

	AC_CHECK_LIB(Xss, XScreenSaverQueryInfo,
	[
		AC_CHECK_HEADER(X11/extensions/scrnsaver.h,
		[
			AC_DEFINE(HAVE_LIBXSS, 1, [define if you have Xss])
			XSS_LIBS="-lX11 -lXss"
			LIBS="$LIBS $XSS_LIBS"
			have_libXss=yes
			AC_MSG_RESULT([yes])
		])
	])
fi

 

####################
# Check for others #
####################
AC_C_BIGENDIAN

AM_ICONV_LINK

LIBS="$LIBS $LIBICONV"

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([short])

AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)

AX_WITH_CURSES
if test "x$ax_cv_ncurses" = "xyes"
then
                LIBS="$LIBS $CURSES_LIB"
else
                AC_MSG_ERROR([You need ncurses/ncursesw library and header files.])
fi


AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>
],[ socklen_t foo; ], ac_cv_c_socklen_t=yes, ac_cv_c_socklen_t=no)

if test "x$ac_cv_c_socklen_t" = "xno"; then
  AC_DEFINE(socklen_t, int, [Define type of socklen_t for systems missing it])
fi

case "$host_os" in
  darwin*)
        LIBS="$LIBS -flat_namespace"
        CXXFLAGS="$CXXFLAGS -I/sw/include"
        ;;
esac

AM_CONDITIONAL(BUILD_ICQ, test "x$build_icq" = "xyes")
AM_CONDITIONAL(BUILD_YAHOO, test "x$build_yahoo" = "xyes")
AM_CONDITIONAL(BUILD_AIM, test "x$build_aim" = "xyes")
AM_CONDITIONAL(BUILD_IRC, test "x$build_irc" = "xyes")
AM_CONDITIONAL(BUILD_FIRETALK, test "x$build_irc" = "xyes" -o "x$build_aim" = "xyes")
AM_CONDITIONAL(BUILD_JABBER, test "x$build_jabber" = "xyes")
AM_CONDITIONAL(BUILD_GADU, test "x$build_gg" = "xyes")
AM_CONDITIONAL(BUILD_MSN, test "x$build_msn" = "xyes")
AM_CONDITIONAL(BUILD_RSS, test "x$build_rss" = "xyes" -o "x$build_lj" = "xyes")
AM_CONDITIONAL(BUILD_LJ, test "x$build_lj" = "xyes")
AM_CONDITIONAL(HAVE_THREAD, test "x$pthread" = "xyes")
AM_CONDITIONAL(HAVE_LIBOTR, test "x$have_libotr" = "xyes")
AM_CONDITIONAL(HAVE_FRIBIDI, test "x$have_fribidi" = "xyes")

if test "x$build_icq" = "xyes"; then
	AC_DEFINE(BUILD_ICQ, 1, [build with ICQ support])
fi
if test "x$build_yahoo" = "xyes"; then
	if test "x$have_curl" = "xno"; then
		AC_MSG_ERROR([Yahoo cannot be built without the curl library.])
	fi
	if test -z "$libcurl_feature_SSL" -o "x$libcurl_feature_SSL" = "xno"; then
		AC_MSG_ERROR([Yahoo requires libcurl built with SSL support.])
	fi
	AC_DEFINE(BUILD_YAHOO, 1, [build with yahoo support])
fi
if test "x$build_aim" = "xyes"; then
	AC_DEFINE(BUILD_AIM, 1, [build with aim support])
fi
if test "x$build_irc" = "xyes";
	then AC_DEFINE(BUILD_IRC, 1, [build with irc support])
fi
if test "x$build_jabber" = "xyes"; then
	if test "x$have_gpgme" = "xno"; then
        	AC_MSG_WARN([No GPG support in Jabber, since GPGME library not found or its setup not ok])
	fi
	AC_DEFINE(BUILD_JABBER, 1, [build with jabber support])
fi
if test "x$build_gg" = "xyes"; then
	if test "x$have_libjpeg" = "xno"; then
        	AC_MSG_WARN([No jpeg support in Gadu-Gadu, you will not be able to register??????????])
	fi
	AC_DEFINE(BUILD_GADU, 1, [build with gadu-gadu support])
fi
if test "x$build_msn" = "xyes"; then
	if test "x$have_curl" = "xno"; then
		AC_MSG_ERROR([MSN cannot be built without the curl library.])
	fi
	if test -z "$libcurl_feature_SSL" -o "x$libcurl_feature_SSL" = "xno"; then
		AC_MSG_ERROR([MSN requires libcurl built with SSL support.])
	fi
	AC_DEFINE(BUILD_MSN, 1, [build with MSN support])
fi
if test "x$build_rss" = "xyes" -o "x$build_lj" = "xyes"; then
	AC_DEFINE(BUILD_RSS, 1, [build the integrated rss feeds reader])
fi
if test "x$build_lj" = "xyes"; then
	AC_DEFINE(BUILD_LJ, 1, [build the integrated livejournal client])
fi
if test "x$locales_fx" != "xno"; then
  AC_DEFINE([LOCALES_HACK], 1, [enables workaround for broken locales])
fi

############################
# Checks for header files. #
############################
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([argz.h])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([ftw.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([langinfo.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdio_ext.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/mount.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/statvfs.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/vfs.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([utmp.h])
AC_CHECK_HEADERS([wchar.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([_G_config.h])

AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_CHECK_HEADERS([vector iostream fstream string algorithm list memory iterator exception utility],, AC_MSG_ERROR([You need to have the libstdc++ headers installed]))
AC_CHECK_HEADERS([hash_map strstream ext/hash_map sstream],,)
AC_LANG_RESTORE

##################################################
# check whether make supports order-only targets #
##################################################

AC_CACHE_CHECK([whether ${MAKE-make} supports order-only targets],
[centerim_cv_make_order_only],
[cat >conftest.make <<\_ACEOF
SHELL = /bin/sh
all: | say

say:
	echo say

.PHONY: all say
_ACEOF
centerim_cv_make_order_only=no

${MAKE-make} -f conftest.make >/dev/null 2>&1 && centerim_cv_make_order_only=yes
rm -f conftest.make
])
if test x$centerim_cv_make_order_only = xyes; then
  ORDER_ONLY="|"
fi


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

AC_SUBST(GNUTLS_LIBS)
AC_SUBST(GNUTLS_CFLAGS) 
AC_SUBST(GNUTLS_CXXFLAGS) 

AC_SUBST(CURL_LIBS)
AC_SUBST(CURL_CFLAGS) 
AC_SUBST(CURL_CXXFLAGS) 

AC_SUBST(JPEG_LIBS)

AC_SUBST(ORDER_ONLY)

#################################
# Checks for library functions. #
#################################
AC_FUNC_SELECT_ARGTYPES

AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([getpass])
AC_CHECK_FUNCS([inet_aton])
AC_CHECK_FUNCS([inet_ntoa])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strtol])


########################
# Create output files. #
########################
ALL_LINGUAS="zh_TW zh_CN uk sv ru ro pt_BR pl nl ms it hu fr es de cs bg"
AC_CONFIG_FILES([Makefile
                 connwrap/Makefile
                 firetalk/Makefile
                 kkconsui/Makefile
                 kkconsui/include/Makefile
                 kkconsui/src/Makefile
                 kkstrtext/Makefile
                 kksystr/Makefile
                 kksystr/include/Makefile
                 kksystr/src/Makefile
                 libgadu/Makefile
                 libicq2000/Makefile
                 libicq2000/libicq2000/Makefile
                 libicq2000/src/Makefile
                 libjabber/Makefile
                 libmsn/Makefile
                 libmsn/msn/Makefile
                 libyahoo2/Makefile
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
  CFLAGS                $CFLAGS
  CXXFLAGS              $CXXFLAGS

Protocols and Clients:])
if test "x$build_icq" = "xyes"; then AC_MSG_RESULT([  ICQ]); fi
if test "x$build_yahoo" = "xyes"; then AC_MSG_RESULT([  Yahoo]); fi
if test "x$build_aim" = "xyes"; then AC_MSG_RESULT([  AIM]); fi
if test "x$build_jabber" = "xyes"; then AC_MSG_RESULT([  jabber]); fi
if test "x$build_gg" = "xyes"; then AC_MSG_RESULT([  Gadu-Gadu]); fi
if test "x$build_msn" = "xyes"; then AC_MSG_RESULT([  MSN Messenger]); fi
if test "x$build_rss" = "xyes"; then AC_MSG_RESULT([  RSS Reader]); fi
if test "x$build_lj" = "xyes"; then AC_MSG_RESULT([  LiveJournal client]); fi
if test "x$have_fribidi" = "xyes"; then AC_MSG_RESULT([  LiveJournal client]); fi
if test "x$with_ssl" = "xno"; then 
AC_MSG_RESULT([
WARNING: SSL disabled (This means no Google Talk)
]);
else
if test "x$have_gpgme" = "xno"; then
AC_MSG_RESULT([
WARNING: GPGME disabled (This means no encrypted messages between users)
]); fi
if test "x$have_libotr" = "xyes"; then
AC_MSG_RESULT([
WARNING: libotr enabled (OTR is not supported yet, use at your own risk)
]); fi
if test "x$pthread" = "xno"; then
AC_MSG_RESULT([
WARNING: pthreads disabled (No file-transfers in jabber)
]); fi
fi
AC_MSG_RESULT([
Enjoy!])
