INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/libyahoo -I/home/konst/devel/libmsn -I/home/konst/devel/libicq2000 -I$(top_srcdir)/intl
CPPFLAGS = -DDEBUG
noinst_LIBRARIES = libhooks.a
libhooks_a_SOURCES = icqhook.cc yahoohook.cc abstracthook.cc msnhook.cc
EXTRA_DIST = icqhook.cc yahoohook.cc abstracthook.cc msnhook.cc abstracthook.h icqhook.h yahoohook.h msnhook.h
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(datadir)/locale\" @SIGC_CFLAGS@
