INCLUDES = -I/home/konst/devel/icqlib -I/home/konst/devel/kktoolbox/kkconsui -I/home/konst/devel/kktoolbox/kkstrtext -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc
centericq_LDFLAGS = -L/home/konst/devel/icqlib -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kkstrtext
centericq_LDADD = -lncurses -licqlib -lkkconsui -lkkstrtext @LIBS@ @INTLLIBS@
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc centericq.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqhist.h icqhook.h icqmlist.h icqoffline.h
AUTOMAKE_OPTIONS = foreign
