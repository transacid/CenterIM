INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/libmsn -I/home/konst/devel/libicq2000/include -I/home/konst/devel/libicq2000 -I/home/konst/devel/firetalk -I/home/konst/devel/libyahoo2 -I/home/konst/devel/libjabber -I$(top_srcdir)/intl
CPPFLAGS = -DDEBUG -DPACKETDEBUG
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqmlist.cc icqgroup.cc icqgroups.cc imcontact.cc accountmanager.cc imcontroller.cc imevents.cc eventmanager.cc imlogger.cc imexternal.cc
centericq_LDFLAGS = -L/home/konst/devel/kktoolbox/kkstrtext -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/libmsn -L/home/konst/devel/libicq2000 -L/home/konst/devel/firetalk -L/home/konst/devel/libyahoo2 -L/home/konst/devel/libjabber
centericq_LDADD = $(top_srcdir)/src/hooks/libhooks.a  -lkkstrtext -lkkconsui -lkksystr -llibmsn -llibicq2000 -lfiretalk -llibyahoo2 -llibjabber  @INTLLIBS@
SUBDIRS = hooks
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqmlist.cc icqgroup.cc icqgroups.cc imcontact.cc accountmanager.cc imcontroller.cc imevents.cc eventmanager.cc imlogger.cc imexternal.cc centericq.h icqcommon.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqmlist.h icqgroup.h icqgroups.h accountmanager.h imcontact.h imcontroller.h imevents.h eventmanager.h imlogger.h imexternal.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"
LDFLAGS = $(GLIB_LIBS)
