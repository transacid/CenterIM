noinst_LIBRARIES = liblibjabber.a
EXTRA_DIST = expat.c genhash.c hashtable.c jconn.c jid.c jpacket.c jutil.c log.c pool.c pproxy.c rate.c sha.c snprintf.c socket.c str.c xmlnode.c xmlparse.c xmlrole.c xmltok.c xstream.c asciitab.h hashtable.h iasciitab.h jabber.h latin1tab.h libxode.h log.h nametab.h utf8tab.h xmldef.h xmlparse.h xmlrole.h xmltok.h xmltok_impl.h xmltok_impl_c.h xmltok_ns_c.h
AUTOMAKE_OPTIONS = foreign

if BUILD_JABBER

liblibjabber_a_SOURCES =  expat.c genhash.c hashtable.c jconn.c jid.c jpacket.c jutil.c log.c pool.c pproxy.c rate.c sha.c snprintf.c socket.c str.c xmlnode.c xmlparse.c xmlrole.c xmltok.c xstream.c

else

liblibjabber_a_LIBADD = dummy.o

dummy.c:
	touch dummy.c

endif

SUBDIRS = 
INCLUDES = -I$(top_srcdir)/../connwrap -I$(top_srcdir)/../connwrap-0.1
CPPFLAGS = 
