AM_CXXFLAGS = -Wno-write-strings

centerim_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/hooks -I$(top_srcdir)/connwrap -I$(top_srcdir)/firetalk -I$(top_srcdir)/firetalk/../connwrap -I$(top_srcdir)/firetalk/../connwrap-0.1 -I$(top_srcdir)/kkconsui -I$(top_srcdir)/kkconsui/include -I$(top_srcdir)/kkconsui/../kkstrtext -I$(top_srcdir)/kkconsui/../kkstrtext-0.1 -I$(top_srcdir)/kkstrtext -I$(top_srcdir)/kksystr -I$(top_srcdir)/kksystr/include -I$(top_srcdir)/kksystr/../kkstrtext -I$(top_srcdir)/kksystr/../kkstrtext-0.1 -I$(top_srcdir)/libgadu -I$(top_srcdir)/libicq2000 -I$(top_srcdir)/libicq2000/libicq2000 -I$(top_srcdir)/libjabber -I$(top_srcdir)/libjabber/../connwrap -I$(top_srcdir)/libjabber/../connwrap-0.1 -I$(top_srcdir)/libmsn -I$(top_srcdir)/libmsn/../connwrap -I$(top_srcdir)/libmsn/../connwrap-0.1 -I$(top_srcdir)/libyahoo2 -I$(top_srcdir)/libyahoo2/../connwrap -I$(top_srcdir)/libyahoo2/../connwrap-0.1 -I$(top_srcdir)/intl
bin_PROGRAMS = centerim
centerim_SOURCES = centerim.cc icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc icqcommon.cc imcontroller.cc icqconf.cc imevents.cc impgp.cc icqface.cc centermain.cc icqgroup.cc captcha.cc git-version.cc
centerim_LDFLAGS = -L$(top_builddir)/connwrap -L$(top_builddir)/kkconsui -L$(top_builddir)/kkstrtext -L$(top_builddir)/kksystr -L$(top_builddir)/libicq2000
centerim_LDADD = $(top_builddir)/src/hooks/libhooks.a  -lkkconsui -lkkstrtext -lkksystr -llibicq2000
if HAVE_LIBOTR
centerim_SOURCES += imotr.cc
endif
if HAVE_FRIBIDI
centerim_LDADD +=$(FRIBIDI_LIBS)
#centerim_CFLAGS += $(FRIBIDI_LIBS)
endif
if BUILD_FIRETALK
centerim_LDFLAGS += -L$(top_builddir)/firetalk
centerim_LDADD += -lfiretalk
endif
if BUILD_GADU
centerim_LDFLAGS += -L$(top_builddir)/libgadu
centerim_LDADD += -llibgadu
endif
if BUILD_JABBER
centerim_LDFLAGS += -L$(top_builddir)/libjabber
centerim_LDADD += -llibjabber
endif
if BUILD_MSN
centerim_LDFLAGS += -L$(top_builddir)/libmsn
centerim_LDADD += -llibmsn
endif
if BUILD_YAHOO
centerim_LDFLAGS += -L$(top_builddir)/libyahoo2
centerim_LDADD += -llibyahoo2
endif
centerim_LDADD += -lconnwrap @LTLIBINTL@
SUBDIRS = hooks
EXTRA_DIST = centerim.cc icqcontact.cc icqgroups.cc eventmanager.cc icqdialogs.cc icqhist.cc imexternal.cc imcontact.cc imlogger.cc accountmanager.cc icqcontacts.cc icqmlist.cc imcontroller.cc icqconf.cc imevents.cc impgp.cc imotr.cc icqface.cc centermain.cc icqgroup.cc icqmlist.h imcontroller.h icqcommon.h icqcontacts.h centerim.h icqgroup.h imcontact.h accountmanager.h eventmanager.h imevents.h icqcontact.h imlogger.h icqconf.h impgp.h imotr.h icqface.h icqgroups.h icqhist.h imexternal.h captcha.h git-version.h.in
AUTOMAKE_OPTIONS = foreign
centerim_CPPFLAGS += -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"

# regenerate git-version.h if its content differs from the output of
# git-version-gen
g_get_cur = sed -ne "s/^\#[ \x09]*define[ \x09]\+GIT_VERSION[ \x09]\+\"\([0-9]\+.*\)\"/\1/p" \
             2>/dev/null < git-version.h | tr -d '\012'
g_subst_f = sed -e "s/@GIT_VERSION@/$$VER_GIT/" < git-version.h.in > git-version.h


# If the make we are using supports order-only targets we make git_version is
# made an order-only prerequisite.  Order-only prerequisites are build before
# the target(s) but do not lead to a rebuild of the target(s).
ORDER_ONLY = @ORDER_ONLY@
$(centerim_OBJECTS) git-version.h : $(ORDER_ONLY) git_version
.PHONY: git_version

git_version: git-version.h.in
	VER_FILE=`$(g_get_cur)` || true; \
	VER_GIT=`cd $(top_srcdir); ./misc/git-version-gen .version`; \
	if [ -z "$$VER_FILE" ] || [ "$$VER_FILE" != "$$VER_GIT" ]; then \
		$(g_subst_f); \
	fi
