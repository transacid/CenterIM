EXTRA_DIST = libmsn.motor ChangeLog AUTHORS COPYING INSTALL NEWS README
SUBDIRS = msn
INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
AM_CPPFLAGS = 
noinst_LIBRARIES = liblibmsn.a
AUTOMAKE_OPTIONS = foreign

if BUILD_MSN

liblibmsn_a_SOURCES = 
liblibmsn_a_LIBADD =  msn/authdata.o msn/buddy.o msn/connection.o msn/filetransfer.o msn/invitation.o msn/message.o msn/notificationserver.o msn/passport.o msn/switchboardserver.o msn/util.o

else

liblibmsn_a_SOURCES = dummy.c
liblibmsn_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif
