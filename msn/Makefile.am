INCLUDES = -I$(top_srcdir)/msn/.. -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = 
noinst_LIBRARIES = libmsn.a
EXTRA_DIST = message.cpp authdata.cpp switchboardserver.cpp invitation.cpp notificationserver.cpp filetransfer.cpp buddy.cpp connection.cpp util.cpp passport.cpp msn.h authdata.h errorcodes.h filetransfer.h switchboardserver.h util.h externals.h invitation.h message.h connection.h notificationserver.h buddy.h passport.h
SUBDIRS = 

if BUILD_MSN

libmsn_a_SOURCES =  message.cpp authdata.cpp switchboardserver.cpp invitation.cpp notificationserver.cpp filetransfer.cpp buddy.cpp connection.cpp util.cpp passport.cpp

else

libmsn_a_LIBADD = dummy.o

dummy.cpp:
	touch dummy.cpp

endif
