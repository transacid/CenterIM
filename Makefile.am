noinst_LIBRARIES = libfiretalk.a
EXTRA_DIST = aim.c firetalk.c irc.c oscar.c safestring.c toc.c aim.h firetalk-int.h firetalk.h irc.h oscar.h safestring.h toc.h firetalk.motor INSTALL NEWS README COPYING AUTHORS ChangeLog
AUTOMAKE_OPTIONS = foreign

if BUILD_FIRETALK

libfiretalk_a_SOURCES =  aim.c firetalk.c irc.c oscar.c safestring.c toc.c

else

libfiretalk_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif

SUBDIRS = 
INCLUDES = -I$(top_srcdir)/../connwrap
CPPFLAGS = 
