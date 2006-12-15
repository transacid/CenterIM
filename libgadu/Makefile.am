EXTRA_DIST = libgadu.c pubdir.c dcc.c obsolete.c common.c http.c events.c pubdir50.c libgadu.h libgadu-config.h compat.h libgadu.motor COPYING
SUBDIRS = 
INCLUDES = 
CPPFLAGS = 
noinst_LIBRARIES = liblibgadu.a

if BUILD_GADU

liblibgadu_a_SOURCES =  libgadu.c pubdir.c dcc.c obsolete.c common.c http.c events.c pubdir50.c

else

liblibgadu_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif

AUTOMAKE_OPTIONS = foreign
