INCLUDES = -I/home/konst/devel/ktools -I/home/konst/devel/ktools/include -I/home/konst/devel/icqlib -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc
centericq_LDFLAGS = -L/home/konst/devel/ktools -L/home/konst/devel/icqlib
centericq_LDADD = -lktools -lcurses -licqlib @LIBS@ @INTLLIBS@
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc centericq.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqhist.h icqhook.h icqmlist.h icqoffline.h
AUTOMAKE_OPTIONS = foreign
CXXFLAGS = -g -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(datadir)/locale\"
