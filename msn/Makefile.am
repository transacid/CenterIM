INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = 
noinst_LIBRARIES = libmsn.a
EXTRA_DIST = authdata.cpp buddy.cpp connection.cpp filetransfer.cpp invitation.cpp message.cpp notificationserver.cpp passport.cpp switchboardserver.cpp util.cpp authdata.h buddy.h connection.h errorcodes.h externals.h filetransfer.h invitation.h message.h msn.h notificationserver.h passport.h sstream_fix.h switchboardserver.h util.h
SUBDIRS = 

if BUILD_MSN

libmsn_a_SOURCES =  authdata.cpp buddy.cpp connection.cpp filetransfer.cpp invitation.cpp message.cpp notificationserver.cpp passport.cpp switchboardserver.cpp util.cpp

else

libmsn_a_LIBADD = dummy.o

dummy.cpp:
	touch dummy.cpp

endif
