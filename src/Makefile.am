INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I/home/konst/devel/kktoolbox/kkconsui -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/kktoolbox/kkconsui/../kkstrtext -I/home/konst/devel/kktoolbox/kkconsui/../kkstrtext-0.1 -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/kktoolbox/kksystr -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/kktoolbox/kksystr/../kkstrtext-0.1 -I/home/konst/devel/kktoolbox/kksystr/../kkstrtext -I/home/konst/devel/libicq2000 -I/home/konst/devel/libmsn -I/home/konst/devel/firetalk -I/home/konst/devel/libyahoo2 -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = accountmanager.cc centericq.cc centermain.cc eventmanager.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqgroup.cc icqgroups.cc icqhist.cc icqmlist.cc imcontact.cc imcontroller.cc imevents.cc imexternal.cc imlogger.cc
centericq_LDFLAGS = -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kkstrtext -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/libicq2000 -L/home/konst/devel/libmsn -L/home/konst/devel/firetalk -L/home/konst/devel/libyahoo2
centericq_LDADD = $(top_srcdir)/src/hooks/libhooks.a  -lkkconsui -lkkstrtext -lkksystr -llibicq2000 -llibmsn -lfiretalk -llibyahoo2 @LIBS@ @INTLLIBS@
SUBDIRS = hooks
EXTRA_DIST = accountmanager.cc centericq.cc centermain.cc eventmanager.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqgroup.cc icqgroups.cc icqhist.cc icqmlist.cc imcontact.cc imcontroller.cc imevents.cc imexternal.cc imlogger.cc accountmanager.h centericq.h eventmanager.h icqcommon.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqgroup.h icqgroups.h icqhist.h icqmlist.h imcontact.h imcontroller.h imevents.h imexternal.h imlogger.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\" $(GLIB_CFLAGS) @SIGC_CFLAGS@
LDFLAGS = @SIGC_LIBS@ $(GLIB_LIBS)
