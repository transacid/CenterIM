INCLUDES = -I/home/konst/devel/kktoolbox/kkstrtext -I/home/konst/devel/icqlib -I/home/konst/devel/kktoolbox/kkconsui/include -I/home/konst/devel/kktoolbox/kksystr/include -I$(top_srcdir)/intl
bin_PROGRAMS = centericq
centericq_SOURCES = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc
centericq_LDFLAGS = -L/home/konst/devel/kktoolbox/kkstrtext -L/home/konst/devel/kktoolbox/kkconsui -L/home/konst/devel/kktoolbox/kksystr -L/home/konst/devel/icqlib
centericq_LDADD = -lkkstrtext -lkkconsui -lkksystr -licqlib -lcurses @LIBS@ @INTLLIBS@
EXTRA_DIST = centericq.cc centermain.cc icqconf.cc icqcontact.cc icqcontacts.cc icqdialogs.cc icqface.cc icqhist.cc icqhook.cc icqmlist.cc icqoffline.cc centericq.h icqconf.h icqcontact.h icqcontacts.h icqface.h icqhist.h icqhook.h icqmlist.h icqoffline.h
AUTOMAKE_OPTIONS = foreign
