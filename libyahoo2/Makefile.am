noinst_LIBRARIES = liblibyahoo2.a
EXTRA_DIST = crypt.c libyahoo2.c sha.c yahoo_list.c yahoo_httplib.c yahoo_util.c yahoo_fn.c sha.h yahoo2_callbacks.h yahoo_debug.h yahoo_httplib.h yahoo2_types.h yahoo_list.h yahoo_util.h yahoo2.h yahoo_fn.h libyahoo2.motor
AUTOMAKE_OPTIONS = foreign

if BUILD_YAHOO

liblibyahoo2_a_SOURCES =  crypt.c libyahoo2.c sha.c yahoo_list.c yahoo_httplib.c yahoo_util.c yahoo_fn.c

else

liblibyahoo2_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif

SUBDIRS = 
INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = -DSTDC_HEADERS -DUSE_STRUCT_CALLBACKS
