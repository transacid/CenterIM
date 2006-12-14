noinst_LIBRARIES = libfiretalk.a
EXTRA_DIST = aim.c irc.c toc.c firetalk.c safestring.c oscar.c aim.h irc.h toc.h firetalk-int.h firetalk.h safestring.h oscar.h toc2_uuids.h
AUTOMAKE_OPTIONS = foreign

if BUILD_FIRETALK

libfiretalk_a_SOURCES =  aim.c irc.c toc.c firetalk.c safestring.c oscar.c

else

libfiretalk_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif

SUBDIRS = 
INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = 
