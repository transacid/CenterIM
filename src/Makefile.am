INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I/home/konst/devel/icqlib -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/libyahoo -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqmlist.cc icqoffline.cc icqgroup.cc icqgroups.cc imcontact.cc accountmanager.cc imcontroller.cc
centericq_LDFLAGS = -L/home/konst/devel/icqlib -L/home/konst/devel/kktoolbox/kkstrtext -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/libyahoo
centericq_LDADD = $(top_srcdir)/src/hooks/libhooks.a -licqlib -lkkstrtext -lkksystr -lkkconsui -llibyahoo @LIBS@ @INTLLIBS@
SUBDIRS = hooks
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqmlist.cc icqoffline.cc icqgroup.cc icqgroups.cc imcontact.cc accountmanager.cc imcontroller.cc centericq.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqhist.h icqmlist.h icqoffline.h icqcommon.h icqgroup.h icqgroups.h imcontact.h accountmanager.h imcontroller.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(datadir)/locale\"
