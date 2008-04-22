AM_CPPFLAGS = -I$(top_srcdir)//connwrap -I$(top_srcdir)/libmsn
noinst_LIBRARIES = libmsn.a

if BUILD_MSN

noinst_HEADERS =authdata.h buddy.h connection.h errorcodes.h externals.h filetransfer.h invitation.h md5.h message.h msn.h notificationserver.h passport.h sstream_fix.h switchboardserver.h util.h
libmsn_a_SOURCES =  authdata.cpp buddy.cpp connection.cpp filetransfer.cpp invitation.cpp message.cpp notificationserver.cpp passport.cpp switchboardserver.cpp util.cpp

else

libmsn_a_LIBADD = dummy.o

dummy.cpp:
	touch dummy.cpp

endif
