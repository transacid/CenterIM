INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
noinst_LIBRARIES = liblibyahoo2.a

if BUILD_YAHOO

liblibyahoo2_a_SOURCES =  crypt.c yahoo_httplib.c md5.c libyahoo2.c yahoo_list.c yahoo_util.c

endif

EXTRA_DIST = crypt.c yahoo_httplib.c md5.c libyahoo2.c yahoo_list.c yahoo_util.c yahoo_httplib.h md5.h libyahoo2.h yahoo2_callbacks.h yahoo_list.h yahoo_util.h yahoo2_types.h libyahoo2.motor
AUTOMAKE_OPTIONS = foreign
