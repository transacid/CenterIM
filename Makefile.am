EXTRA_DIST = libmsn.motor NEWS README AUTHORS INSTALL ChangeLog COPYING
SUBDIRS = msn
INCLUDES = -I$(top_srcdir)/msn/.. -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = 
noinst_LIBRARIES = liblibmsn.a
AUTOMAKE_OPTIONS = foreign

if BUILD_MSN

liblibmsn_a_LIBADD =  msn/message.o msn/authdata.o msn/switchboardserver.o msn/invitation.o msn/notificationserver.o msn/filetransfer.o msn/buddy.o msn/connection.o msn/util.o msn/passport.o

else

liblibmsn_a_SOURCES = 

dummy.cpp:
	touch dummy.cpp

endif
