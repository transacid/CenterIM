#ifndef __ICQOFFLINE_H_
#define __ICQOFFLINE_H_

#include <stdio.h>

#include "icq.h"
#include "icqhook.h"
#include "contactinfo.h"

#include "icqcommon.h"

enum scanaction {
    osresend,
    ossendall,
    osremove,
    osexpired
};

class icqoffline {
    protected:
	int processed, totalunsent;

	FILE *open(contactinfo cinfo, const char *mode);

	bool sendevent(contactinfo cinfo, bool msg, string url, string text,
	    FILE *of, unsigned long seq, time_t tm, scanaction act,
	    unsigned long sseq);

    public:
	icqoffline();
	~icqoffline();

	void sendmsg(contactinfo cinfo, const string text);
	void sendurl(contactinfo cinfo, const string url, const string text);

	void scan(unsigned long sseq, scanaction act);

	int getunsentcount();
};

extern icqoffline offl;

#endif
