INCLUDES = -I/home/konst/devel/icqlib -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/kktoolbox/kksystr/include -I/home/konst/devel/kktoolbox/kksystr -I/home/konst/devel/kktoolbox/kkstrtext -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc
centericq_LDFLAGS = -L/home/konst/devel/icqlib -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/kktoolbox/kkstrtext
centericq_LDADD = -licqlib -lkkconsui -lkksystr -lcurses -lkkstrtext @LIBS@ @INTLLIBS@
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc centericq.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqhist.h icqhook.h icqmlist.h icqoffline.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(datadir)/locale\"
