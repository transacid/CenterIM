dist-hook:
	cd $(top_distdir); subs=`egrep "^[:space:]*SUBDIRS" Makefile.am | sed 's/SUBDIRS[ =]\+//g'`; \
	echo "SUBDIRS = connwrap-0.1 firetalk-0.1 kkconsui-0.1 kkstrtext-0.1 kksystr-0.1 libgadu-0.1 libicq2000-0.1 libjabber-0.1 libmsn-0.1 libyahoo2-0.1 $$subs" >>.makefile.am; \
	egrep -v "^[:space:]*SUBDIRS" Makefile.am >>.makefile.am; \
	mv .makefile.am Makefile.am;  autoconf && automake

# END OF MOTOR DIST TARGETS #
EXTRA_DIST = POLICY centericq.1 centericq.motor FAQ NEWS TODO README THANKS AUTHORS INSTALL ABOUT-NLS ChangeLog COPYING centericq.spec connwrap-0.1/* firetalk-0.1/* kkconsui-0.1/* kkstrtext-0.1/* kksystr-0.1/* libgadu-0.1/* libicq2000-0.1/* libjabber-0.1/* libmsn-0.1/* libyahoo2-0.1/*
SUBDIRS = misc share src
AUTOMAKE_OPTIONS = foreign
