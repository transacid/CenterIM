##
## Process with automake to produce Makefile.in
##

# Headers are installed in ${pkgincludedir}/libicq2000, this will allow
# multiple versions of the library to be installed in future without
# their headers overlapping (by varying the pkgincludedir with
# versioning information). Also it means a client compiling with the
# library will have to explicitly use the -I flag for the library
# headers it needs, guaranteeing it knows what it's getting, typically
# this will be got from the libicq2000-config script.

headersincludedir = $(pkgincludedir)/libicq2000

headersinclude_HEADERS = \
 Client.h       ContactTree.h  sigslot.h \
 constants.h    events.h       time_extra.h         version.h \
 Contact.h      exceptions.h   Translator.h \
 ContactList.h  ref_ptr.h      userinfoconstants.h
EXTRA_DIST = Contact.h constants.h Translator.h time_extra.h events.h ref_ptr.h Client.h ContactList.h exceptions.h ContactTree.h userinfoconstants.h version.h sigslot.h
SUBDIRS = 
