#ifndef __ICQOFFLINE_H_
#define __ICQOFFLINE_H_

#include <stdio.h>

#include "icqhook.h"
#include "imcontact.h"

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

	FILE *open(imcontact cinfo, const char *mode);

	bool sendevent(imcontact cinfo, bool msg, string url, string text,
	    FILE *of, unsigned long seq, time_t tm, scanaction act,
	    unsigned long sseq);

    public:
	icqoffline();
	~icqoffline();

	void sendmsg(imcontact cinfo, const string text, FILE *of = 0);
	void sendurl(imcontact cinfo, const string url, const string text, FILE *of = 0);

	void scan(unsigned long sseq, scanaction act);

	int getunsentcount();
};

extern icqoffline offl;

#endif
