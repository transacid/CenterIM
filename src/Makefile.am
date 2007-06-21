INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I$(top_srcdir)/connwrap -I$(top_srcdir)/firetalk -I$(top_srcdir)/firetalk/../connwrap -I$(top_srcdir)/firetalk/../connwrap-0.1 -I$(top_srcdir)/kkconsui -I$(top_srcdir)/kkconsui/include -I$(top_srcdir)/kkconsui/../kkstrtext -I$(top_srcdir)/kkconsui/../kkstrtext-0.1 -I$(top_srcdir)/kkstrtext -I$(top_srcdir)/kksystr -I$(top_srcdir)/kksystr/include -I$(top_srcdir)/kksystr/../kkstrtext -I$(top_srcdir)/kksystr/../kkstrtext-0.1 -I$(top_srcdir)/libgadu -I$(top_srcdir)/libicq2000 -I$(top_srcdir)/libicq2000/libicq2000 -I$(top_srcdir)/libjabber -I$(top_srcdir)/libjabber/../connwrap -I$(top_srcdir)/libjabber/../connwrap-0.1 -I$(top_srcdir)/libmsn -I$(top_srcdir)/libmsn/../connwrap -I$(top_srcdir)/libmsn/../connwrap-0.1 -I$(top_srcdir)/libyahoo2 -I$(top_srcdir)/libyahoo2/../connwrap -I$(top_srcdir)/libyahoo2/../connwrap-0.1 -I$(top_srcdir)/intl
bin_PROGRAMS = centerim
centerim_SOURCES = centerim.cc icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc imcontroller.cc icqconf.cc imevents.cc impgp.cc icqface.cc centermain.cc icqgroup.cc captcha.cc
centerim_LDFLAGS = -L$(top_builddir)/connwrap -L$(top_builddir)/kkconsui -L$(top_builddir)/kkstrtext -L$(top_builddir)/kksystr -L$(top_builddir)/libicq2000
centerim_LDADD = $(top_builddir)/src/hooks/libhooks.a  -lconnwrap -lkkconsui -lkkstrtext -lkksystr -llibicq2000
if BUILD_FIRETALK
centerim_LDFLAGS += -L$(top_builddir)/firetalk
centerim_LDADD += -lfiretalk
endif
if BUILD_GADU
centerim_LDFLAGS += -L$(top_builddir)/libgadu
centerim_LDADD += -llibgadu
endif
if BUILD_JABBER
centerim_LDFLAGS += -L$(top_builddir)/libjabber
centerim_LDADD += -llibjabber
endif
if BUILD_MSN
centerim_LDFLAGS += -L$(top_builddir)/libmsn
centerim_LDADD += -llibmsn
endif
if BUILD_YAHOO
centerim_LDFLAGS += -L$(top_builddir)/libyahoo2
centerim_LDADD += -llibyahoo2
endif
centerim_LDADD += @LTLIBINTL@
SUBDIRS = hooks
EXTRA_DIST = centerim.cc icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc imcontroller.cc icqconf.cc imevents.cc impgp.cc icqface.cc centermain.cc icqgroup.cc icqmlist.h imcontroller.h icqcommon.h icqcontacts.h centerim.h icqgroup.h imcontact.h accountmanager.h eventmanager.h imevents.h icqcontact.h imlogger.h icqconf.h impgp.h icqface.h icqgroups.h icqhist.h imexternal.h captcha.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"
