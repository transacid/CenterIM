#ifndef __ICQOFFLINE_H_
#define __ICQOFFLINE_H_

#include <stdio.h>
#include <string>
#include <vector>

#include "icq.h"
#include "icqhook.h"

enum scanaction {osresend, ossendall, osremove, osexpired};

class icqoffline {
    protected:
	int processed, totalunsent;

	FILE *open(unsigned int uin, const char *mode);

	bool sendevent(unsigned int uin, bool msg, string url, string text,
	    FILE *of, unsigned long seq, time_t tm, scanaction act,
	    unsigned long sseq);

    public:
	icqoffline();
	~icqoffline();

	void sendmsg(unsigned int uin, string text);
	void sendurl(unsigned int uin, string url, string text);
	void scan(unsigned long seq, scanaction act);

	int getunsentcount();
};

extern icqoffline offl;

#endif
