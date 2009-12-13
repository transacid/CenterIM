#AUTOMAKE=@AUTOMAKE@ --add-missing --foreign
#ACLOCAL=@ACLOCAL@ -I config

WANTED_PROTOCOLS = 
if BUILD_MSN 
WANTED_PROTOCOLS += libmsn
endif
if BUILD_YAHOO
WANTED_PROTOCOLS += libyahoo2
endif
if BUILD_FIRETALK
WANTED_PROTOCOLS += firetalk
endif
if BUILD_GADU
WANTED_PROTOCOLS += libgadu
endif
if BUILD_JABBER
WANTED_PROTOCOLS += libjabber
endif
SUBDIRS = \
          connwrap \
          kkstrtext \
          kksystr \
          kkconsui \
          libicq2000 \
          $(WANTED_PROTOCOLS) \
          intl \
          po \
          misc \
          share \
          src
EXTRA_DIST = \
             config.rpath \
             POLICY \
             centerim.1 \
             FAQ \
             NEWS \
             TODO \
             README \
             THANKS \
             AUTHORS \
             INSTALL \
             ABOUT-NLS \
             ChangeLog \
             COPYING
man_MANS = centerim.1

ACLOCAL_AMFLAGS = -I m4

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

