INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I/home/konst/devel/firetalk -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/libicq2000 -I/home/konst/devel/libjabber -I/home/konst/devel/libyahoo2 -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/connwrap -I/home/konst/devel/libgadu -I/home/konst/devel/blip -I/home/konst/devel/libicq2000/libicq2000 -I$(top_srcdir)/intl
CPPFLAGS = -DDEBUG
bin_PROGRAMS = centericq
centericq_SOURCES = icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc imcontroller.cc icqconf.cc imevents.cc icqface.cc centermain.cc icqgroup.cc centericq.cc
centericq_LDFLAGS = -L/home/konst/devel/firetalk -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kkstrtext -L/home/konst/devel/libicq2000 -L/home/konst/devel/libjabber -L/home/konst/devel/libyahoo2 -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/connwrap -L/home/konst/devel/libgadu -L/home/konst/devel/blip
centericq_LDADD = $(top_srcdir)/src/hooks/libhooks.a  -lfiretalk -lkkconsui -lkkstrtext -llibicq2000 -llibjabber -llibyahoo2 -lkksystr -lconnwrap -llibgadu -lblip @INTLLIBS@
SUBDIRS = hooks
EXTRA_DIST = icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc imcontroller.cc icqconf.cc imevents.cc icqface.cc centermain.cc icqgroup.cc centericq.cc icqmlist.h imcontroller.h icqcommon.h icqcontacts.h centericq.h icqgroup.h imcontact.h accountmanager.h eventmanager.h imevents.h icqcontact.h imlogger.h icqconf.h icqface.h icqgroups.h icqhist.h imexternal.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"
